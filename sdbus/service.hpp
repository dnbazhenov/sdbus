#ifndef SDBUS_SERVICE_HPP_
#define SDBUS_SERVICE_HPP_

#include "sdbus/forwards.hpp"

#include <systemd/sd-bus-protocol.h>
#include <systemd/sd-bus.h>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/associated_cancellation_slot.hpp>
#include <boost/asio/detail/fenced_block.hpp>
#include <boost/asio/detail/handler_work.hpp>
#include <boost/asio/detail/io_object_impl.hpp>
#include <boost/asio/detail/reactor.hpp>
#include <boost/intrusive/list.hpp>
#include <sdbus/sdbus.hpp>

#include <cmath>
#include <functional>
#include <list>
#include <system_error>
#include <utility>

namespace boost::asio::sdbus
{

class message
{
    struct ref_wrapper
    {
        ref_wrapper(sd_bus_message*& ref) : _ref(ref), _new(ref)
        {}
        ~ref_wrapper()
        {
            if (_ref)
            {
                sd_bus_message_unref(_ref);
            }
            _ref = _new;
        }
        sd_bus_message*& _ref;
        sd_bus_message* _new;

        operator sd_bus_message**()
        {
            return &_new;
        }
    };

  public:
    struct move_tag
    {};

    constexpr message() = default;
    constexpr message(sd_bus_message* m) : _m(sd_bus_message_ref(m))
    {
        printf("MESSAGE(%p): CONSTRUCT m=%p\n", static_cast<void*>(this), static_cast<void*>(_m));
    }
    constexpr message(move_tag, sd_bus_message* m) : _m(m)
    {
        printf("MESSAGE(%p): CONSTRUCT m=%p\n", static_cast<void*>(this), static_cast<void*>(_m));
    }
    constexpr message(message&& m) : _m(m._m)
    {
        printf("MESSAGE(%p): MOVE CONSTRUCT m=%p o = %p\n", static_cast<void*>(this),
               static_cast<void*>(m._m), static_cast<void*>(&m));
        m._m = nullptr;
    }
    message(const message& m) : _m(sd_bus_message_ref(m._m))
    {
        printf("MESSAGE(%p): COPY CONSTRUCT m=%p\n", static_cast<void*>(this),
               static_cast<void*>(_m));
    }
    ~message()
    {
        printf("MESSAGE(%p): DESTRUCT m=%p\n", static_cast<void*>(this), static_cast<void*>(_m));
        sd_bus_message_unref(_m);
    }
    const char* get_signature(int complete = 1) const
    {
        return sd_bus_message_get_signature(_m, complete);
    }
    message& operator=(message&& m)
    {
        printf("MESSAGE(%p): MOVE FROM m=%p o = %p\n", static_cast<void*>(this),
               static_cast<void*>(m._m), static_cast<void*>(&m));
        _m = m._m;
        m._m = nullptr;
        return *this;
    }
    operator bool() const
    {
        return !!_m;
    }
    operator sd_bus_message*() const
    {
        return _m;
    }
    template <typename... T>
    void append(T&&... v) const
    {
        (append_one(std::forward<T>(v)), ...);
    }

    template <typename T>
    void append_one(T&& v) const
    {
        ::sdbus::write(_m, std::forward<T>(v));
    }
    ref_wrapper reference()
    {
        return ref_wrapper(_m);
    }

    template <typename T>
    T read() const
    {
        T v;
        ::sdbus::read(_m, v);
        return v;
    }

    struct container_scope
    {
        container_scope() = delete;
        container_scope(const message& m) : _m(m)
        {}
        container_scope(const container_scope&) = delete;
        container_scope(container_scope&& s) : _m(s._m)
        {
            s._exit = false;
        }
        ~container_scope()
        {
            if (_exit)
            {
                sd_bus_message_exit_container(_m);
            }
        }

      private:
        const message& _m;
        bool _exit = true;
    };

    container_scope enter_array() const
    {
        return enter_container(SD_BUS_TYPE_ARRAY);
    }

    bool at_end(bool complete = false) const
    {
        auto err = sd_bus_message_at_end(_m, complete);
        if (err < 0)
        {
            throw std::system_error(std::error_code(err, std::system_category()));
        }
        return static_cast<bool>(err);
    }

  private:
    container_scope enter_container(char type) const
    {
        auto err = sd_bus_message_enter_container(_m, type, nullptr);
        if (err < 0)
        {
            throw std::system_error(std::error_code(err, std::system_category()));
        }

        return container_scope(*this);
    }

  private:
    sd_bus_message* _m = nullptr;
};

namespace detail
{

using namespace boost::asio::detail;

class bus_state;

/*
 * Base class for captured async operations.
 */
class read_op_base : public operation
{
  public:
    using operation::operation;

    void set_message(message&& m)
    {
        _message = std::move(m);
    }

    void set_cancel_key(void* key)
    {
        _key = key;
    }

    void* get_cancel_key() const
    {
        return _key;
    }

  protected:
    void* _key;
    message _message;
};

/*
 * Captured async operation.
 */
template <typename Handler, typename IoExecutor>
class read_op : public read_op_base
{
  public:
    read_op(Handler& h, const IoExecutor& ex) :
        read_op_base(&read_op::do_complete), _handler(std::move(h)), _work(_handler, ex)
    {}

    BOOST_ASIO_DEFINE_HANDLER_PTR(read_op);

  private:
    static void do_complete(void* owner, operation* base, const boost::system::error_code&,
                            std::size_t)
    {
        BOOST_ASIO_ASSUME(base != 0);

        auto o = static_cast<read_op*>(base);

        BOOST_ASIO_HANDLER_COMPLETION((*o));

        auto w = std::move(o->_work);
        auto handler =
            move_binder1<Handler, message>(0, std::move(o->_handler), std::move(o->_message));

        ptr{boost::asio::detail::addressof(handler.handler_), o, o}.reset();

        if (owner)
        {
            fenced_block b(fenced_block::half);
            BOOST_ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_));
            w.complete(handler, handler.handler_);
            BOOST_ASIO_HANDLER_INVOCATION_END;
        }
    }

  private:
    Handler _handler;
    handler_work<Handler, IoExecutor> _work;
};

/*
 * Maintained slot state.
 */
class slot_state
{
  public:
    slot_state(bus_state& s);
    ~slot_state();

    void set_slot(sd_bus_slot* slot)
    {
        _slot = slot;
    }

    bool operator==(const slot_state& s) const
    {
        return s._slot == _slot;
    }

    void destroy();
    void cancel();
    void cancel_by_key(void* key);

    void push_message(sdbus::message m);
    void start_op(read_op_base* op, bool is_continuation);

  private:
    // owner bus
    bus_state& _bus_state;
    // sd-bus slot pointer
    sd_bus_slot* _slot = nullptr;
    // undelivered messages
    std::list<message> _messages;
    // pending read operations
    op_queue<operation> _ops;
    // state lock
    mutex _mutex;
};

/*
 * Maintained bus state.
 */
class bus_state : public reactor_op, public boost::intrusive::list_base_hook<>
{
  public:
    bus_state(sd_bus* bus, reactor& reactor, scheduler& sched);
    ~bus_state();

    bool operator==(const bus_state& s) const
    {
        return s._bus == _bus;
    }

    scheduler& get_sched() const
    {
        return _sched;
    }

    sd_bus* get_bus() const
    {
        return _bus;
    }
    void shutdown();
    void cancel();
    void cancel_by_key(void* key);
    slot_state* call(const message& m, u_int64_t usec);
    slot_state* add_match(const std::string_view& match);
    void remove_slot(slot_state* state);

  private:
    static status do_perform(reactor_op* op);
    static void do_complete(void*, operation*, const boost::system::error_code&, std::size_t);
    static int slot_callback(sd_bus_message* m, void* userdata, sd_bus_error*);
    static int install_callback(sd_bus_message* m, void* userdata, sd_bus_error*);

  private:
    using slot_states = std::list<slot_state>;

    // associated bus object
    sd_bus* _bus = nullptr;
    // reactor service
    reactor& _reactor;
    // scheduler service
    scheduler& _sched;
    // outstanding matches and calls
    slot_states _states;
    // reactor data
    reactor::per_descriptor_data _reactor_data;
    // state lock
    mutex _mutex;
};

/*
 * Bus service.
 */
class bus_service : public execution_context_service_base<bus_service>
{
  public:
    struct implementation_type
    {
        bus_state* state = nullptr;
    };

    bus_service(execution_context& context) :
        execution_context_service_base<bus_service>(context),
        _reactor(use_service<reactor>(context)), _scheduler(use_service<scheduler>(context))
    {
        printf("BUS_SERVICE: INIT\n");
        _reactor.init_task();
    }

    void shutdown() override
    {
        printf("BUS_SERVICE: SHUTDOWN\n");
        mutex::scoped_lock lock(_mutex);
        while (_states.size())
        {
            remove_state(_states.begin());
        }
    }

    void construct(implementation_type&)
    {
        printf("BUS_SERVICE: CONSTRUCT\n");
    }

    static void move_construct(implementation_type& impl, implementation_type& other_impl)
    {
        impl.state = other_impl.state;
        other_impl.state = nullptr;
    }

    void destroy(implementation_type& impl)
    {
        if (impl.state == nullptr)
        {
            return;
        }

        mutex::scoped_lock lock(_mutex);
        auto& state = *impl.state;
        _states.remove(state);
        state.shutdown();
    }

    void assign(implementation_type& impl, sd_bus* bus)
    {
        printf("BUS_SERVICE: ASSIGN(%p)\n", static_cast<void*>(bus));
        destroy(impl);
        impl.state = add_state(bus);
    }

  private:
    using states = boost::intrusive::list<bus_state>;

    bus_state* add_state(sd_bus* bus)
    {
        mutex::scoped_lock lock(_mutex);
        auto state = new bus_state(bus, _reactor, _scheduler);
        _states.push_back(*state);
        printf("BUS_SERVICE: ADD_STATE(bus=%p, state=%p)\n", static_cast<void*>(bus),
               static_cast<void*>(state));
        return state;
    }

    void remove_state(states::iterator iter)
    {
        auto& state = *iter;
        _states.erase(iter);
        state.shutdown();
    }

  private:
    // reactor service
    reactor& _reactor;
    // scheduler service
    scheduler& _scheduler;
    // bus states
    states _states;
    // list mutex
    mutex _mutex;
};

/*
 * Bus service.
 */
class slot_service : public execution_context_service_base<slot_service>
{
    class read_op_cancellation;

  public:
    struct implementation_type
    {
        slot_state* state = nullptr;
    };

    slot_service(execution_context& context) : execution_context_service_base<slot_service>(context)
    {
        printf("SLOT_SERVICE: INIT\n");
    }

    void shutdown() override
    {
        printf("SLOT_SERVICE: SHUTDOWN\n");
        // NOTE: nothing to do
    }

    void construct(implementation_type&)
    {}

    static void move_construct(implementation_type& impl, implementation_type& other_impl)
    {
        impl.state = other_impl.state;
        other_impl.state = nullptr;
    }

    void destroy(implementation_type& impl)
    {
        if (impl.state)
        {
            impl.state->destroy();
        }
    }

    void assign(implementation_type& impl, slot_state* state)
    {
        printf("SLOT_SERVICE: ASSIGN state=%p\n", static_cast<void*>(state));
        destroy(impl);
        impl.state = state;
    }

    // Start an asynchronous operation to wait for a signal to be delivered.
    template <typename Handler, typename IoExecutor>
    void async_read(implementation_type& impl, Handler& handler, const IoExecutor& io_ex)
    {
        bool is_continuation = boost_asio_handler_cont_helpers::is_continuation(handler);

        typename associated_cancellation_slot<Handler>::type slot =
            get_associated_cancellation_slot(handler);

        // Allocate and construct an operation to wrap the handler.
        typedef read_op<Handler, IoExecutor> op;
        typename op::ptr p = {boost::asio::detail::addressof(handler), op::ptr::allocate(handler),
                              0};
        p.p = new (p.v) op(handler, io_ex);

        // Optionally register for per-operation cancellation.
        if (slot.is_connected())
        {
            p.p->set_cancel_key(&slot.template emplace<read_op_cancellation>(this, &impl));
        }

        BOOST_ASIO_HANDLER_CREATION(
            (scheduler_.context(), *p.p, "signal_set", &impl, 0, "async_read"));

        impl.state->start_op(p.p, is_continuation);
        p.v = p.p = 0;
    }

  private:
    class read_op_cancellation
    {
      public:
        read_op_cancellation(slot_service* s, implementation_type* i) : _service(s), _impl(i)
        {}

        void operator()(cancellation_type_t type)
        {
            if (!!(type & (cancellation_type::terminal | cancellation_type::partial |
                           cancellation_type::total)))
            {
                //                _impl->state->cancel_by_key(_impl->slot, this);
            }
        }

      private:
        slot_service* _service;
        implementation_type* _impl;
    };
};

} // namespace detail

template <typename Executor>
class bus;

template <typename Executor>
class slot
{
    class initiate_async_read;
    friend class bus<Executor>;

  public:
    /// The type of the executor associated with the object.
    using executor_type = Executor;

    /// Rebinds the slot to another executor.
    template <typename Executor1>
    struct rebind_executor
    {
        /// The signal set type when rebound to the specified executor.
        typedef slot<Executor1> other;
    };

    /// Construct an empty slot object.
    explicit slot(const executor_type& ex) : _impl(0, ex)
    {}

    template <BOOST_ASIO_COMPLETION_TOKEN_FOR(void(message))
                  SignalToken = default_completion_token_t<executor_type>>
    auto async_read(SignalToken&& token = default_completion_token_t<executor_type>())
        -> decltype(async_initiate<SignalToken, void(message)>(declval<initiate_async_read>(),
                                                               token))
    {
        return async_initiate<SignalToken, void(message)>(initiate_async_read(this), token);
    }

  private:
    class initiate_async_read
    {
      public:
        using executor_type = Executor;

        explicit initiate_async_read(slot* self) : _self(self)
        {}

        const executor_type& get_executor() const noexcept
        {
            return _self->get_executor();
        }

        template <typename Handler>
        void operator()(Handler&& handler) const
        {
            detail::non_const_lvalue<Handler> handler2(handler);
            _self->_impl.get_service().async_read(_self->_impl.get_implementation(), handler2.value,
                                                  _self->_impl.get_executor());
        }

      private:
        slot* _self;
    };

  private:
    /// Construct a slot object bound to slot state.
    slot(const executor_type& ex, detail::slot_state* state) : _impl(0, ex)
    {
        printf("SLOT: CONSTRUCT slot_state=%p\n", static_cast<void*>(state));
        _impl.get_service().assign(_impl.get_implementation(), state);
    }

  private:
    detail::io_object_impl<detail::slot_service, Executor> _impl;
};

template <typename Executor = any_io_executor>
class bus
{
  public:
    /// The type of the executor associated with the object.
    using executor_type = Executor;

    /// Rebinds the slot to another executor.
    template <typename Executor1>
    struct rebind_executor
    {
        /// The signal set type when rebound to the specified executor.
        typedef bus<Executor1> other;
    };

    /// Construct an empty bus object.
    explicit bus(const executor_type& ex) : _impl(0, ex)
    {}

    /// Construct an empty bus object.
    template <typename ExecutionContext>
    explicit bus(ExecutionContext& context,
                 std::enable_if_t<is_convertible<ExecutionContext&, execution_context&>::value,
                                  defaulted_constraint> = defaulted_constraint()) :
        _impl(0, 0, context)
    {}

    void bus_default()
    {
        sd_bus* b;
        sd_bus_default_system(&b);
        _impl.get_service().assign(_impl.get_implementation(), b);
        printf("BUS: DEFAULT bus=%p state=%p\n", static_cast<void*>(b),
               static_cast<void*>(_impl.get_implementation().state));
    }

    /// Construct a default system bus.
    template <typename ExecutorOrExecutionContext>
    static bus bus_default_system(ExecutorOrExecutionContext&& ex)
    {
        sd_bus* b;
        sd_bus_default_system(&b);
        return bus{std::forward<ExecutorOrExecutionContext>, b};
    }

    /// Construct a default user bus.
    template <typename ExecutorOrExecutionContext>
    static bus bus_default_user(ExecutorOrExecutionContext&& ex)
    {
        sd_bus* b;
        sd_bus_default_user(&b);
        return bus{std::forward<ExecutorOrExecutionContext>, b};
    }

    /// Add new match rule.
    slot<executor_type> add_match(const std::string_view& match_string)
    {
        return {_impl.get_executor(), _impl.get_implementation()->add_match(match_string)};
    }

    /// Invoke a D-Bus method call.
    slot<executor_type> call(const message& m, u_int64_t usec = 0)
    {
        printf("BUS: CALL state=%p\n", static_cast<void*>(_impl.get_implementation().state));
        return slot{_impl.get_executor(), _impl.get_implementation().state->call(m, usec)};
    }

    template <typename... Args>
    message new_method_call(std::string_view service, std::string_view object_path,
                            std::string_view interface, std::string_view method, Args&&... args)
    {
        auto s = _impl.get_implementation().state;
        sd_bus_message* m;

        printf("BUS: NEW_METHOD_CALL state=%p\n",
               static_cast<void*>(_impl.get_implementation().state));

        int ret = sd_bus_message_new_method_call(
            s->get_bus(), &m, service.data(), object_path.data(), interface.data(), method.data());
        message m1({}, m);
        if (ret >= 0)
        {
            m1.append(std::forward<Args>(args)...);
        }

        printf("SIG: %s\n", sd_bus_message_get_signature(m, 1));
        return m1;
    }

  private:
    /// Construct a bus object bound to sd_bus pointer.
    explicit bus(const executor_type& ex, sd_bus* b) : _impl(0, ex)
    {
        _impl.get_service().assign(_impl.get_implementation(), b);
    }

    /// Construct a bus object bound to sd_bus pointer.
    template <typename ExecutionContext>
    explicit bus(ExecutionContext& context, sd_bus* b,
                 std::enable_if_t<is_convertible<ExecutionContext&, execution_context&>::value,
                                  defaulted_constraint> = defaulted_constraint()) :
        _impl(0, 0, context)
    {
        _impl.get_service().assign(_impl.get_implementation(), b);
    }

  private:
    detail::io_object_impl<detail::bus_service, Executor> _impl;
};

} // namespace boost::asio::sdbus

#endif // SDBUS_SERVICE_HPP_

#include <sdbus/service.hpp>

#include <utility>

namespace boost::asio::sdbus::detail
{

static const system::error_code success_ec;

slot_state::slot_state(bus_state& s) : _bus_state(s)
{
    // printf("SLOT_STATE: CONSTRUCT state=%p bus_state=%p\n", static_cast<void*>(this),
    //        static_cast<void*>(&s));
}

slot_state::~slot_state()
{
    // printf("SLOT_STATE: DESTRUCT state=%p\n", static_cast<void*>(this));
    sd_bus_slot_set_userdata(_slot, nullptr);
    sd_bus_slot_unref(_slot);
}

void slot_state::destroy()
{
    // printf("SLOT_STATE: DESTROY state=%p\n", static_cast<void*>(this));
    _bus_state.remove_slot(this);
}

void slot_state::cancel()
{
    // printf("SLOT_STATE: CANCEL state=%p\n", static_cast<void*>(this));
    mutex::scoped_lock lock(_mutex);
    _bus_state.get_sched().post_deferred_completions(_ops);
}

void slot_state::cancel_by_key(void* key)
{
    op_queue<operation> ops;
    mutex::scoped_lock lock(_mutex);

    auto op = static_cast<read_op_base*>(_ops.front());

    while (op)
    {
        auto next = op_queue_access::next(op);

        if (op->get_cancel_key() == key)
        {
            ops.push(op);
        }

        op = next;
    }

    if (!ops.empty())
    {
        _bus_state.get_sched().post_deferred_completions(_ops);
    }
}

void slot_state::push_message(sdbus::message m)
{
    // printf("SLOT_STATE: PUSH_MSG state=%p m=%p\n", static_cast<void*>(this),
    //        static_cast<void*>(&m));
    mutex::scoped_lock lock(_mutex);

    if (m.get_signature() == std::string_view("s"))
    {
        std::string s = m.read<std::string>();
        printf("S: %s\n", s.data());
    }
    if (_ops.empty())
    {
        // printf("SLOT_STATE: PUSH state=%p m=%p\n", static_cast<void*>(this),
        //        static_cast<void*>(&m));
        _messages.push_back(m);
    }
    else
    {
        auto op = static_cast<read_op_base*>(_ops.front());
        _ops.pop();
        // printf("SLOT_STATE: POST_WORK state=%p op=%p\n", static_cast<void*>(this),
        //        static_cast<void*>(op));
        op->set_message(std::move(m));
        _bus_state.get_sched().post_deferred_completion(op);
    }
}

void slot_state::start_op(read_op_base* op, bool is_continuation)
{
    // printf("BUS_STATE: START_OP state=%p op=%p\n", static_cast<void*>(this),
    //        static_cast<void*>(op));
    mutex::scoped_lock lock(_mutex);

    if (_messages.size())
    {
        op->set_message(std::move(_messages.front()));
        _messages.pop_front();
        _bus_state.get_sched().post_immediate_completion(op, is_continuation);
    }
    else
    {
        // printf("SLOT_STATE: WORK_STARTED state=%p\n", static_cast<void*>(this));
        _bus_state.get_sched().work_started();
        _ops.push(op);
    }
}

bus_state::bus_state(sd_bus* bus, reactor& reactor, scheduler& sched) :
    reactor_op(success_ec, &bus_state::do_perform, &bus_state::do_complete), _bus(bus),
    _reactor(reactor), _sched(sched)
{
    // printf("BUS_STATE: CONSTRUCT state=%p bus=%p\n", static_cast<void*>(this),
    //        static_cast<void*>(bus));
    _reactor.register_internal_descriptor(reactor::read_op, sd_bus_get_fd(_bus), _reactor_data,
                                          this);
}

bus_state::~bus_state()
{
    sd_bus_unref(_bus);
}

void bus_state::shutdown()
{
    bool destroy = false;
    {
        mutex::scoped_lock lock(_mutex);

        _reactor.deregister_internal_descriptor(sd_bus_get_fd(_bus), _reactor_data);
        _reactor.cleanup_descriptor_data(_reactor_data);

        for (auto& state : _states)
        {
            state.cancel();
        }

        destroy = _states.empty();
    }

    if (destroy)
    {
        delete this;
    }
}

void bus_state::cancel()
{
    mutex::scoped_lock lock(_mutex);
    for (auto& state : _states)
    {
        state.cancel();
    }
}

void bus_state::cancel_by_key(void* key)
{
    mutex::scoped_lock lock(_mutex);

    for (auto& state : _states)
    {
        state.cancel_by_key(key);
    }
}

slot_state* bus_state::call(const message& m, u_int64_t usec)
{
    mutex::scoped_lock lock(_mutex);

    _states.emplace_back(*this);

    auto& state = _states.back();
    sd_bus_slot* s;

    auto ret = sd_bus_call_async(_bus, &s, m, &slot_callback, &state, usec);
    // printf("BUS_STATE: CALL state=%p SLOT=%p SLOT_STATE=%p ret=%d\n", static_cast<void*>(this),
    //        static_cast<void*>(s), static_cast<void*>(&state), ret);
    if (ret < 0)
    {
        _states.pop_back();
        errno = ret;
        return nullptr;
    }

    state.set_slot(s);

    return &state;
}

slot_state* bus_state::add_match(const std::string_view& match)
{
    mutex::scoped_lock lock(_mutex);

    _states.emplace_back(*this);

    auto& state = _states.back();
    sd_bus_slot* s;

    auto ret =
        sd_bus_add_match_async(_bus, &s, match.data(), &slot_callback, &install_callback, &state);
    if (ret < 0)
    {
        _states.pop_back();
        errno = ret;
        return nullptr;
    }

    return &state;
}

void bus_state::remove_slot(slot_state* state)
{
    // printf("BUS_STATE: REMOVE_SLOT state=%p\n", static_cast<void*>(state));

    if (state == nullptr)
    {
        return;
    }

    bool destroy = false;

    {
        mutex::scoped_lock lock(_mutex);
        state->cancel();
        _states.remove(*state);
        destroy = _states.empty() && !_reactor_data;
    }

    if (destroy)
    {
        delete this;
    }
}

reactor_op::status bus_state::do_perform(reactor_op* op)
{
    auto state = static_cast<bus_state*>(op);

    mutex::scoped_lock lock(state->_mutex);

    for (;;)
    {
        auto ret = sd_bus_process(state->_bus, nullptr);
        // printf("BUS_STATE: DO_PERFORM op=%p ret=%d\n", static_cast<void*>(op), ret);
        if (ret > 0)
        {
            continue;
        }

        break;
    }

    return not_done;
}

void bus_state::do_complete(void*, [[maybe_unused]] operation* op, const boost::system::error_code&,
                            std::size_t)
{
    // printf("BUS_STATE: DO_COMPLETE op=%p\n", static_cast<void*>(op));
}

int bus_state::slot_callback(sd_bus_message* m, void* userdata, sd_bus_error*)
{
    // printf("SLOT_CALLBACK: userdata=%p\n", userdata);

    if (userdata == nullptr)
    {
        return 0;
    }

    auto state = static_cast<slot_state*>(userdata);

    state->push_message(m);

    return 0;
}

int bus_state::install_callback(sd_bus_message* m, void* userdata, sd_bus_error*)
{
    // printf("INSTALL_CALLBACK: userdata=%p\n", userdata);
    if (userdata == nullptr)
    {
        return 0;
    }

    if (!sd_bus_message_get_error(m))
    {
        return 0;
    }

    auto state = static_cast<slot_state*>(userdata);

    state->push_message(m);

    return 0;
}

} // namespace boost::asio::sdbus::detail

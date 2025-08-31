#ifndef SDBUS_MESSAGE_HPP_
#define SDBUS_MESSAGE_HPP_

#include <systemd/sd-bus.h>

#include <sdbus/sdbus.hpp>

#include <system_error>

namespace sdbus
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
        // printf("MESSAGE(%p): CONSTRUCT m=%p\n", static_cast<void*>(this),
        // static_cast<void*>(_m));
    }

    constexpr message(move_tag, sd_bus_message* m) : _m(m)
    {
        // printf("MESSAGE(%p): CONSTRUCT m=%p\n", static_cast<void*>(this),
        // static_cast<void*>(_m));
    }
    constexpr message(message&& m) : _m(m._m)
    {
        // printf("MESSAGE(%p): MOVE CONSTRUCT m=%p o = %p\n", static_cast<void*>(this),
        //        static_cast<void*>(m._m), static_cast<void*>(&m));
        m._m = nullptr;
    }
    message(const message& m) : _m(sd_bus_message_ref(m._m))
    {
        // printf("MESSAGE(%p): COPY CONSTRUCT m=%p\n", static_cast<void*>(this),
        //        static_cast<void*>(_m));
    }
    ~message()
    {
        // printf("MESSAGE(%p): DESTRUCT m=%p\n", static_cast<void*>(this), static_cast<void*>(_m));
        sd_bus_message_unref(_m);
    }
    const char* get_signature(int complete = 1) const
    {
        return sd_bus_message_get_signature(_m, complete);
    }
    message& operator=(message&& m)
    {
        // printf("MESSAGE(%p): MOVE FROM m=%p o = %p\n", static_cast<void*>(this),
        //        static_cast<void*>(m._m), static_cast<void*>(&m));
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
        sdbus::write(_m, std::forward<T>(v));
    }
    ref_wrapper reference()
    {
        return ref_wrapper(_m);
    }

    template <typename T>
    T read() const
    {
        T v;
        sdbus::read(_m, v);
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

} // namespace sdbus

#endif /* SDBUS_MESSAGE_HPP_ */

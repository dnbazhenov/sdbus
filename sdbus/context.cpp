#include <sdbus/context.hpp>

namespace sdbus
{

static constexpr auto DBUS_MAX_PATH = 256;

static thread_local char path[DBUS_MAX_PATH];

static char* r_dump_entry(const path_entry& e)
{
    if (&e.parent == &e)
    {
        return stpcpy(path, "<top>");
    }

    char* r = r_dump_entry(e.parent);

    size_t remaining = sizeof(path) - (r - path);

    if (e.type == path_entry::array_item)
    {
        return r + snprintf(r, remaining, "/%ld", e.index);
    }
    else
    {
        return r + snprintf(r, remaining, "/%s", e.entry);
    }
}

const char* path_entry::to_str() const
{
    r_dump_entry(*this);
    return path;
}

errc sdbus_errc(int ret)
{
    if (ret == -ENXIO)
    {
        return errc::invalid_type;
    }
    if (ret == -ENOMEM)
    {
        return errc::no_memory;
    }
    if (ret < 0)
    {
        return errc::write_error;
    }
    return errc::success;
}

} // namespace sdbus

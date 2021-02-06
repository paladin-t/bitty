inline void ipv4copy(ipv4 *dst, struct sockaddr *addr)
{
    *dst = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
}

inline void ipv6copy(ipv6 *dst, struct sockaddr *addr)
{
    memcpy(dst, ((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr, sizeof(ipv6));
}

inline int wcsempty(const wchar_t *str)
{
    return wcslen(str) == 0;
}

inline void wszcopy(wchar_t *dst, const wchar_t *src, size_t dst_size)
{
    wcsncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

inline void mbswszcopy(wchar_t *dst, const char *src, size_t dst_size)
{
    mbstowcs(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

/* See https://github.com/jystic/network-info/issues/17 */

extern inline void ipv4copy(ipv4 *dst, struct sockaddr *addr);
extern inline void ipv6copy(ipv6 *dst, struct sockaddr *addr);
extern inline int wcsempty(const wchar_t *str);
extern inline void wszcopy(wchar_t *dst, const wchar_t *src, size_t dst_size);
extern inline void mbswszcopy(wchar_t *dst, const char *src, size_t dst_size);

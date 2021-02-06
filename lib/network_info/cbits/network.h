#define NAME_SIZE (128+4)
#define MAC_SIZE 6

typedef long ipv4;
typedef long ipv6[4];

struct network_interface {
    wchar_t name[NAME_SIZE];
    ipv4 ip_address;
    ipv6 ip6_address;
    unsigned char mac_address[MAC_SIZE];
};

int c_get_network_interfaces(struct network_interface *ns, int max_ns);

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <unistd.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>


#define ATT_CID 4

#define BDADDR_LE_PUBLIC       0x01
#define BDADDR_LE_RANDOM       0x02

struct sockaddr_l2 {
    sa_family_t    l2_family;
    unsigned short l2_psm;
    bdaddr_t       l2_bdaddr;
    unsigned short l2_cid;
    uint8_t        l2_bdaddr_type;
};

#define L2CAP_CONNINFO  0x02

struct l2cap_conninfo {
    uint16_t       hci_handle;
    uint8_t        dev_class[3];
};

static void signalHandler(int signal) {
    printf("got signal %d", signal);
}

int main(int argc, const char* argv[]) {
    //  char *hciDeviceIdOverride = NULL;
    int hciDeviceId = 0;
    char controller_address[18];
    struct hci_dev_info device_info;
    int hciSocket = -1;

    int l2capSock = -1;
    struct sockaddr_l2 sockAddr;
    struct l2cap_conninfo l2capConnInfo;
    socklen_t l2capConnInfoLen;
    int hciHandle;
    int result;

    //  fd_set rfds;
    //  struct timeval tv;

    //  char stdinBuf[256 * 2 + 1];
    //  char l2capSockBuf[256];
    //  int len;
    // int i;
    //  unsigned int data;

    // setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGKILL, signalHandler);
    signal(SIGHUP, signalHandler);
    signal(SIGUSR1, signalHandler);
    signal(SIGUSR2, signalHandler);

    prctl(PR_SET_PDEATHSIG, SIGKILL);

    // open controller
    hciSocket = hci_open_dev(hciDeviceId);
    if (hciSocket == -1) {
        printf("connect hci_open_dev(hci%i): %s\n", hciDeviceId, strerror(errno));
        goto done;
    }

    // get local controller address
    result = hci_devinfo(hciDeviceId, &device_info);
    if (result == -1) {
        printf("connect hci_deviceinfo(hci%i): %s\n", hciDeviceId, strerror(errno));
        goto done;
    }
    ba2str(&device_info.bdaddr, controller_address);
    printf("info using %s@hci%i\n", controller_address, hciDeviceId);

    // create socket
    l2capSock = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (l2capSock  == -1) {
        printf("connect socket(hci%i): %s\n", hciDeviceId, strerror(errno));
        goto done;
    }

    // bind
    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.l2_family = AF_BLUETOOTH;
    // Bind socket to the choosen adapter by using the controllers BT-address as source
    // see l2cap_chan_connect source and hci_get_route in linux/net/bluetooth
    bacpy(&sockAddr.l2_bdaddr, &device_info.bdaddr);
    sockAddr.l2_cid = htobs(ATT_CID);
    result = bind(l2capSock, (struct sockaddr*)&sockAddr, sizeof(sockAddr));
    if (result == -1) {
        printf("connect bind(hci%i): %s\n", hciDeviceId, strerror(errno));
        goto done;
    }

    // connect
    memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.l2_family = AF_BLUETOOTH;
    str2ba(argv[1], &sockAddr.l2_bdaddr);
    sockAddr.l2_bdaddr_type = strcmp(argv[2], "random") == 0 ? BDADDR_LE_RANDOM : BDADDR_LE_PUBLIC;
    sockAddr.l2_cid = htobs(ATT_CID);

    result = connect(l2capSock, (struct sockaddr *)&sockAddr, sizeof(sockAddr));
    if (result == -1) {
        char buf[1024] = { 0 };
        ba2str( &sockAddr.l2_bdaddr, buf );
        printf("connect connect(hci%i): %s\n", hciDeviceId, strerror(errno));
        goto done;
    }

    // get hci_handle
    l2capConnInfoLen = sizeof(l2capConnInfo);
    result = getsockopt(l2capSock, SOL_L2CAP, L2CAP_CONNINFO, &l2capConnInfo, &l2capConnInfoLen);
    if (result == -1) {
        printf("connect getsockopt(hci%i): %s\n", hciDeviceId, strerror(errno));
        goto done;
    }
    hciHandle = l2capConnInfo.hci_handle;

    printf("connect success\n");

    struct bt_security btSecurity;
    socklen_t btSecurityLen;

    int i;

    for(i=0; i < 60;i++) {

        memset(&btSecurity, 0, sizeof(btSecurity));
        btSecurity.level = BT_SECURITY_MEDIUM;

        printf("attempting to increasing socket security level to medium\n");

        result = setsockopt(l2capSock, SOL_BLUETOOTH, BT_SECURITY, &btSecurity, sizeof(btSecurity));
        if (result == -1) {
            printf("security setsockopt(hci%i): %s\n", hciDeviceId, strerror(errno));
            goto done;
        }

//        printf("sleeping for second\n");
        usleep( 20000 );

        printf("verifying socket security level at medium\n");

        result = getsockopt(l2capSock, SOL_BLUETOOTH, BT_SECURITY, &btSecurity, &btSecurityLen);
        if (result == -1) {
            printf("security getsockopt(hci%i): %s\n", hciDeviceId, strerror(errno));
            goto done;
        }

 //       printf("security = %s\n", (BT_SECURITY_MEDIUM == btSecurity.level) ? "medium" : "low");

        if (BT_SECURITY_MEDIUM == btSecurity.level) {
            printf("socket security level at medium\n");
            goto done;
        }
    }

done:
    if (l2capSock != -1)
        close(l2capSock);
    if (hciSocket != -1)
        close(hciSocket);
    printf("disconnect\n");

    return 0;
}

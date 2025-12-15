#include <iostream>
#include <cstring>
#include "HCNetSDK.h"

int main() {
    if (!NET_DVR_Init()) {
        std::cout << "Init failed\n";
        return -1;
    }

    NET_DVR_USER_LOGIN_INFO loginInfo;
    memset(&loginInfo, 0, sizeof(loginInfo));

    strncpy(loginInfo.sDeviceAddress, "10.9.255.21",
            sizeof(loginInfo.sDeviceAddress) - 1);
    loginInfo.wPort = 8000;
    strncpy(loginInfo.sUserName, "admin",
            sizeof(loginInfo.sUserName) - 1);
    strncpy(loginInfo.sPassword, "Wlkjaqxy411",
            sizeof(loginInfo.sPassword) - 1);
    loginInfo.bUseAsynLogin = 0;

    NET_DVR_DEVICEINFO_V40 devInfo;
    memset(&devInfo, 0, sizeof(devInfo));

    LONG userId = NET_DVR_Login_V40(&loginInfo, &devInfo);

    if (userId < 0) {
        std::cout << "Login failed, err="
                  << NET_DVR_GetLastError() << std::endl;
    } else {
        std::cout << "Login success, userId=" << userId << std::endl;
    }

    getchar();
    NET_DVR_Logout(userId);
    NET_DVR_Cleanup();
    return 0;
}

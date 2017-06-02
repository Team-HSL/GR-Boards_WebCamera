#include "mbed.h"
#include "EasyAttach_CameraAndLCD.h"
#include "dcache-control.h"
#include "JPEG_Converter.h"
#include "HTTPServer.h"
#include "mbed_rpc.h"
#include "FATFileSystem.h"
#include "RomRamBlockDevice.h"
#include "SDBlockDevice_GRBoard.h"
#if defined(TARGET_RZ_A1H)
#include "file_table_peach.h"         //Binary data of web pages
#elif defined(TARGET_GR_LYCHEE)
#include "file_table_lychee.h"        //Binary data of web pages
#endif
#include "i2c_setting.h"

/**** User Selection *********/
/** Network setting **/
#define USE_DHCP               (1)                 /* Select  0(static configuration) or 1(use DHCP) */
#if (USE_DHCP == 0)
  #define IP_ADDRESS           ("192.168.0.2")     /* IP address      */
  #define SUBNET_MASK          ("255.255.255.0")   /* Subnet mask     */
  #define DEFAULT_GATEWAY      ("192.168.0.3")     /* Default gateway */
#endif
#define NETWORK_TYPE           (2)                 /* Select  0(Ethernet), 1(BP3595), 2(ESP32 STA) ,3(ESP32 AP) */
#if (NETWORK_TYPE >= 1)
  #define SCAN_NETWORK         (1)                 /* Select  0(Use WLAN_SSID, WLAN_PSK, WLAN_SECURITY) or 1(To select a network using the terminal.) */
  #define WLAN_SSID            ("SSIDofYourAP")    /* SSID */
  #define WLAN_PSK             ("PSKofYourAP")     /* PSK(Pre-Shared Key) */
  #define WLAN_SECURITY        NSAPI_SECURITY_WPA_WPA2 /* NSAPI_SECURITY_NONE, NSAPI_SECURITY_WEP, NSAPI_SECURITY_WPA, NSAPI_SECURITY_WPA2 or NSAPI_SECURITY_WPA_WPA2 */
#endif
/** JPEG out setting **/
#define JPEG_ENCODE_QUALITY    (75)                /* JPEG encode quality (min:1, max:75 (Considering the size of JpegBuffer, about 75 is the upper limit.)) */
#define VFIELD_INT_SKIP_CNT    (0)                 /* A guide for GR-LYCHEE.  0:60fps, 1:30fps, 2:20fps, 3:15fps, 4:12fps, 5:10fps */
/*****************************/

#if (NETWORK_TYPE == 0)
  #include "EthernetInterface.h"
  EthernetInterface network;
#elif (NETWORK_TYPE == 1)
  #include "LWIPBP3595Interface.h"
  LWIPBP3595Interface network;
#elif (NETWORK_TYPE == 2)
  #include "ESP32Interface.h"
  ESP32Interface network(P5_3, P3_14, P3_15, P0_2);
#elif (NETWORK_TYPE == 3)
  #include "ESP32InterfaceAP.h"
  ESP32InterfaceAP network(P5_3, P3_14, P3_15, P0_2);
#else
  #error NETWORK_TYPE error
#endif /* NETWORK_TYPE */

/* Video input and LCD layer 0 output */
#define VIDEO_FORMAT           (DisplayBase::VIDEO_FORMAT_YCBCR422)
#define GRAPHICS_FORMAT        (DisplayBase::GRAPHICS_FORMAT_YCBCR422)
#define WR_RD_WRSWA            (DisplayBase::WR_RD_WRSWA_32_16BIT)
#define DATA_SIZE_PER_PIC      (2u)

/*! Frame buffer stride: Frame buffer stride should be set to a multiple of 32 or 128
    in accordance with the frame buffer burst transfer mode. */
#define VIDEO_PIXEL_HW         (320u)  /* QVGA */
#define VIDEO_PIXEL_VW         (240u)  /* QVGA */

#define FRAME_BUFFER_STRIDE    (((VIDEO_PIXEL_HW * DATA_SIZE_PER_PIC) + 31u) & ~31u)
#define FRAME_BUFFER_HEIGHT    (VIDEO_PIXEL_VW)

DisplayBase Display;

#if defined(__ICCARM__)
#pragma data_alignment=32
static uint8_t user_frame_buffer0[FRAME_BUFFER_STRIDE * FRAME_BUFFER_HEIGHT]@ ".mirrorram";
#else
static uint8_t user_frame_buffer0[FRAME_BUFFER_STRIDE * FRAME_BUFFER_HEIGHT]__attribute((section("NC_BSS"),aligned(32)));
#endif

FATFileSystem fs("storage");
RomRamBlockDevice romram_bd(512000, 512);
SDBlockDevice_GRBoard sd;
Serial pc(USBTX, USBRX);
Thread sdConnectTask;

#if defined(__ICCARM__)
#pragma data_alignment=32
static uint8_t JpegBuffer[2][1024 * 64];
#else
static uint8_t JpegBuffer[2][1024 * 64]__attribute((aligned(32)));
#endif
static size_t jcu_encode_size[2];
static int image_change = 0;
JPEG_Converter Jcu;
static int jcu_buf_index_write = 0;
static int jcu_buf_index_write_done = 0;
static int jcu_buf_index_read = 0;
static int jcu_encoding = 0;
static int Vfield_Int_Cnt = 0;
static char i2c_setting_str_buf[I2C_SETTING_STR_BUF_SIZE];

static void JcuEncodeCallBackFunc(JPEG_Converter::jpeg_conv_error_t err_code) {
    if (err_code == JPEG_Converter::JPEG_CONV_OK) {
        jcu_buf_index_write_done = jcu_buf_index_write;
        image_change = 1;
    }
    jcu_encoding = 0;
}

static int snapshot_req(const char ** pp_data) {
    int encode_size;

    while ((jcu_encoding == 1) || (image_change == 0)) {
        Thread::wait(1);
    }
    jcu_buf_index_read = jcu_buf_index_write_done;
    image_change = 0;

    *pp_data = (const char *)JpegBuffer[jcu_buf_index_read];
    encode_size = (int)jcu_encode_size[jcu_buf_index_read];

    return encode_size;
}

static void IntCallbackFunc_Vfield(DisplayBase::int_type_t int_type) {
    if (Vfield_Int_Cnt < VFIELD_INT_SKIP_CNT) {
        Vfield_Int_Cnt++;
        return;
    }
    Vfield_Int_Cnt = 0;

    //Interrupt callback function
    if (jcu_encoding == 0) {
        JPEG_Converter::bitmap_buff_info_t bitmap_buff_info;
        JPEG_Converter::encode_options_t   encode_options;

        bitmap_buff_info.width              = VIDEO_PIXEL_HW;
        bitmap_buff_info.height             = VIDEO_PIXEL_VW;
        bitmap_buff_info.format             = JPEG_Converter::WR_RD_YCbCr422;
        bitmap_buff_info.buffer_address     = (void *)user_frame_buffer0;

        encode_options.encode_buff_size     = sizeof(JpegBuffer[0]);
        encode_options.p_EncodeCallBackFunc = &JcuEncodeCallBackFunc;
        encode_options.input_swapsetting    = JPEG_Converter::WR_RD_WRSWA_32_16_8BIT;

        jcu_encoding = 1;
        if (jcu_buf_index_read == jcu_buf_index_write) {
            jcu_buf_index_write ^= 1;  // toggle
        }
        jcu_encode_size[jcu_buf_index_write] = 0;
        dcache_invalid(JpegBuffer[jcu_buf_index_write], sizeof(JpegBuffer[0]));
        if (Jcu.encode(&bitmap_buff_info, JpegBuffer[jcu_buf_index_write],
            &jcu_encode_size[jcu_buf_index_write], &encode_options) != JPEG_Converter::JPEG_CONV_OK) {
            jcu_encode_size[jcu_buf_index_write] = 0;
            jcu_encoding = 0;
        }
    }
}

static void Start_Video_Camera(void) {
    // Video capture setting (progressive form fixed)
    Display.Video_Write_Setting(
        DisplayBase::VIDEO_INPUT_CHANNEL_0,
        DisplayBase::COL_SYS_NTSC_358,
        (void *)user_frame_buffer0,
        FRAME_BUFFER_STRIDE,
        VIDEO_FORMAT,
        WR_RD_WRSWA,
        VIDEO_PIXEL_VW,
        VIDEO_PIXEL_HW
    );
    EasyAttach_CameraStart(Display, DisplayBase::VIDEO_INPUT_CHANNEL_0);
}

static void TerminalWrite(Arguments* arg, Reply* r) {
    if ((arg != NULL) && (r != NULL)) {
        for (int i = 0; i < arg->argc; i++) {
            if (arg->argv[i] != NULL) {
                printf("%s", arg->argv[i]);
            }
        }
        printf("\n");
        r->putData<const char*>("ok");
    }
}

static void mount_romramfs(void) {
    FILE * fp;

    romram_bd.SetRomAddr(0x18000000, 0x1FFFFFFF);
    fs.format(&romram_bd, 512);
    fs.mount(&romram_bd);

    //index.htm
    fp = fopen("/storage/index.htm", "w");
    fwrite(index_htm_tbl, sizeof(char), sizeof(index_htm_tbl), fp);
    fclose(fp);

    //camera.js
    fp = fopen("/storage/camera.js", "w");
    fwrite(camaera_js_tbl, sizeof(char), sizeof(camaera_js_tbl), fp);
    fclose(fp);

    //camera.htm
    fp = fopen("/storage/camera.htm", "w");
    fwrite(camera_htm_tbl, sizeof(char), sizeof(camera_htm_tbl), fp);
    fclose(fp);

    //mbedrpc.js
    fp = fopen("/storage/mbedrpc.js", "w");
    fwrite(mbedrpc_js_tbl, sizeof(char), sizeof(mbedrpc_js_tbl), fp);
    fclose(fp);

    //led.htm
    fp = fopen("/storage/led.htm", "w");
    fwrite(led_htm_tbl, sizeof(char), sizeof(led_htm_tbl), fp);
    fclose(fp);

    //i2c_set.htm
    fp = fopen("/storage/i2c_set.htm", "w");
    fwrite(i2c_set_htm_tbl, sizeof(char), sizeof(i2c_set_htm_tbl), fp);
    fclose(fp);

    //web_top.htm
    fp = fopen("/storage/web_top.htm", "w");
    fwrite(web_top_htm_tbl, sizeof(char), sizeof(web_top_htm_tbl), fp);
    fclose(fp);

    //menu.htm
    fp = fopen("/storage/menu.htm", "w");
    fwrite(menu_htm_tbl, sizeof(char), sizeof(menu_htm_tbl), fp);
    fclose(fp);

    //window.htm
    fp = fopen("/storage/window.htm", "w");
    fwrite(window_htm_tbl, sizeof(char), sizeof(window_htm_tbl), fp);
    fclose(fp);
}

static void SetI2CfromWeb(Arguments* arg, Reply* r) {
    int result = 0;

    if (arg != NULL) {
        if (arg->argc >= 2) {
            if ((arg->argv[0] != NULL) && (arg->argv[1] != NULL)) {
                sprintf(i2c_setting_str_buf, "%s,%s", arg->argv[0], arg->argv[1]);
                result = 1;
            }
        } else if (arg->argc == 1) {
            if (arg->argv[0] != NULL) {
                sprintf(i2c_setting_str_buf, "%s", arg->argv[0]);
                result = 1;
            }
        } else {
            /* Do nothing */
        }
        /* command analysis and execute */
        if (result != 0) {
            if (i2c_setting_exe(i2c_setting_str_buf) != false) {
                r->putData<const char*>(i2c_setting_str_buf);
            }
        }
    }
}

#if (SCAN_NETWORK == 1) && (NETWORK_TYPE != 3)
static const char *sec2str(nsapi_security_t sec) {
    switch (sec) {
        case NSAPI_SECURITY_NONE:
            return "None";
        case NSAPI_SECURITY_WEP:
            return "WEP";
        case NSAPI_SECURITY_WPA:
            return "WPA";
        case NSAPI_SECURITY_WPA2:
            return "WPA2";
        case NSAPI_SECURITY_WPA_WPA2:
            return "WPA/WPA2";
        case NSAPI_SECURITY_UNKNOWN:
        default:
            return "Unknown";
    }
}

static bool scan_network(WiFiInterface *wifi) {
    WiFiAccessPoint *ap;
    bool ret = false;
    int i;
    int count = 10;    /* Limit number of network arbitrary to 10 */

    printf("Scan:\r\n");
    ap = new WiFiAccessPoint[count];
    if (ap == NULL) {
        printf("memory error\r\n");
        return 0;
    }
    count = wifi->scan(ap, count);
    for (i = 0; i < count; i++) {
        printf("No.%d Network: %s secured: %s BSSID: %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx RSSI: %hhd Ch: %hhd\r\n",
               i, ap[i].get_ssid(), sec2str(ap[i].get_security()),
               ap[i].get_bssid()[0], ap[i].get_bssid()[1], ap[i].get_bssid()[2], ap[i].get_bssid()[3],
               ap[i].get_bssid()[4], ap[i].get_bssid()[5], ap[i].get_rssi(), ap[i].get_channel());
    }
    printf("%d networks available.\r\n", count);

    if (count > 0) {
        char c;
        char pass[64];
        int select_no;
        bool loop_break = false;;

        printf("\nPlease enter the number of the network you want to connect.\r\n");
        printf("Enter key:[0]-[%d], (If inputting the other key, it's scanned again.)\r\n", count - 1);
        c = (uint8_t)pc.getc();
        select_no = c - 0x30;
        if ((select_no >= 0) && (select_no < count)) {
            printf("[%s] is selected.\r\n", ap[select_no].get_ssid());
            printf("Please enter the PSK.\r\n");
            i = 0;
            while (loop_break == false) {
                c = (uint8_t)pc.getc();
                switch (c) {
                    case 0x0D:
                        pass[i] = '\0';
                        pc.puts("\r\n");
                        loop_break = true;
                        break;
                    case 0x08:
                        if (i > 0) {
                            pc.puts("\b \b");
                            i--;
                        }
                        break;
                    case 0x0A:
                        break;
                    default:
                        if ((i + 1) < sizeof(pass)) {
                            pass[i] = c;
                            i++;
                            pc.putc(c);
                        }
                        break;
                }
            }
            wifi->set_credentials(ap[select_no].get_ssid(), pass, ap[select_no].get_security());
            ret = true;
        }
    }

    delete[] ap;

    return ret;
}
#endif

static void sd_connect_task(void) {
    int storage_type = 0;

    while (1) {
        if (storage_type == 0) {
            if (sd.connect()) {
                fs.unmount();
                fs.mount(&sd);
                storage_type = 1;
                printf("SDBlockDevice\r\n");
            }
        } else {
            if (sd.connected() == false) {
                fs.unmount();
                fs.mount(&romram_bd);
                storage_type = 0;
                printf("RomRamBlockDevice\r\n");
            }
        }
        Thread::wait(250);
    }
}

int main(void) {
    printf("********* PROGRAM START ***********\r\n");

    mount_romramfs();   //RomRamFileSystem Mount

    sdConnectTask.start(&sd_connect_task);

    EasyAttach_Init(Display);
    Jcu.SetQuality(JPEG_ENCODE_QUALITY);
    // Interrupt callback function setting (Field end signal for recording function in scaler 0)
    Display.Graphics_Irq_Handler_Set(DisplayBase::INT_TYPE_S0_VFIELD, 0, IntCallbackFunc_Vfield);
    Start_Video_Camera();

    RPC::add_rpc_class<RpcDigitalOut>();
    RPCFunction rpcFunc(TerminalWrite, "TerminalWrite");
    RPCFunction rpcSetI2C(SetI2CfromWeb, "SetI2CfromWeb");

#if defined(TARGET_RZ_A1H) && (NETWORK_TYPE == 1)
    //Audio Camera Shield USB1 enable for WlanBP3595
    DigitalOut usb1en(P3_8);
    usb1en = 1;        //Outputs high level
    Thread::wait(5);
    usb1en = 0;        //Outputs low level
    Thread::wait(5);
#endif

    printf("Network Setting up...\r\n");
#if (USE_DHCP == 0)
    network.set_dhcp(false);
    if (network.set_network(IP_ADDRESS, SUBNET_MASK, DEFAULT_GATEWAY) != 0) { //for Static IP Address (IPAddress, NetMasks, Gateway)
        printf("Network Set Network Error \r\n");
        return -1;
    }
#endif

#if (NETWORK_TYPE >= 1)
#if (SCAN_NETWORK == 1) && (NETWORK_TYPE != 3)
    while (!scan_network(&network));
#else
    network.set_credentials(WLAN_SSID, WLAN_PSK, WLAN_SECURITY);
#endif
#endif

    printf("\r\nConnecting...\r\n");
    if (network.connect() != 0) {
        printf("Network Connect Error \r\n");
        return -1;
    }
    printf("MAC Address is %s\r\n", network.get_mac_address());
    printf("IP Address is %s\r\n", network.get_ip_address());
    printf("NetMask is %s\r\n", network.get_netmask());
    printf("Gateway Address is %s\r\n", network.get_gateway());
    printf("Network Setup OK\r\n");

    SnapshotHandler::attach_req(&snapshot_req);
    HTTPServerAddHandler<SnapshotHandler>("/camera"); //Camera
    FSHandler::mount("/storage", "/");
    HTTPServerAddHandler<FSHandler>("/");
    HTTPServerAddHandler<RPCHandler>("/rpc");
    HTTPServerStart(&network, 80);
}

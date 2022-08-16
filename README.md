# Neptune_3
  source code

Neptune3-Firmware Update Notes.docx


## 说明
- 请根据以下兼容性关系更新主板和屏幕固件。


## 屏幕固件与主板固件兼容关系
## Compatibility between screen firmware and mainboard firmware
|   屏幕固件版本   |     主板固件版本    | 
|-------------------|--------------------|
| Screen Firmware Version | Mainboard Firmware Version |
| V0 | 停止下载/stop  download |
| V1 | V1_1.0.3 |





## 升级日志/The upgrade log
### 屏幕固件/Screen firmware
#### V1
- 在【设置】中增加【调平温度设置】
- Added 【Leveling Temperature Settings】 to 【Settings】

#### V2 (Stay tuned for)
- @@@
- @@@

### 主板固件/Board firmware
#### V1.0.2
- 增加【调平温度设置】
- 修复了一些已发现bug。改善了调平方式。
- Added 【Leveling Temperature Settings】
- Fixed some discovered bugs. Improved leveling method.

#### V1.0.3
- 修复了一些已发现bug。改善了调平方式。
- Fixed some discovered bugs. Improved leveling method.



## 加载屏幕固件方法
### 1、TF卡格式要求（格式化TF卡）
|   文件系统（F）       | 分配单元大小（A） |
|----------------------|-------------------|
|     FAT32(默认)      |    4096 字节      |

### 2、方法
- ①拆开屏幕外壳。
- ②将文件夹“DWIN_SET”复制到TF卡根目录下，插入屏幕TF卡槽。
- ③重启电源。
- ④等待约1分30秒，屏幕显示第二行会出现“End!”即表示更新屏幕固件成功。
- ⑤移除TF卡，重启电源即可。


## 加载主板固件方法
- ①将文件“ZNP_ROBIN_NANO.bin”复制到TF卡根目录下。
- ②重启电源。
- ③等待约20秒，屏幕显示出现LOGO即表示更新主板固件成功。


## Loading screen firmware method
### 1、TF card format requirements（Format the TF card）
|      File system     |      Allocation unit size     |
|----------------------|-------------------------------|
|      FAT32           |           4096 bit            |

### 2、Method
- ① Remove the screen cover.
- ② Copy the folder 'DWIN_SET' to the TF root directory and insert it into the TF card slot on the screen.
- ③ Restart the power supply.
- ④ After about 90 seconds, the message "End!" is displayed in the second line. The screen firmware is successfully updated.
- ⑤ Remove the TF card and restart the power supply.

## Load motherboard firmware method

- ① Copy the znp_robin_nano-bin file to the TF root directory.
- ② Restart the power supply.
- ③ After about 20 seconds, if the LOGO is displayed on the screen, the mainboard firmware is successfully updated.


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
| V1 |V1.0.2 / V1_1.0.3 |





## 升级日志/The upgrade log
### 屏幕固件/Screen firmware
#### V0
- 旧版本固件不提供下载
- Older firmware versions are not available for download

#### V1
- 在【设置】→【关于】中可以查看屏幕固件版本以及主板固件版本
- 在【设置】中增加【调平温度设置】。默认调平温度：热端140℃  /  热床：60℃
- In 【Settings】→【About】, you can view the screen firmware version and the motherboard firmware version
- Added 【Leveling Temperature Settings】 to 【Settings】. Default leveling temperature: Hotend 140 ° C  / Hotbed 60 ° C


#### V2 (Stay tuned for)
- @@@
- @@@

### 主板固件/Board firmware
#### V1.0.2
- 增加【调平温度设置】
- 修复了一些已发现bug。改善了调平方式。
- Added 【Leveling Temperature Settings】
- Fixed some discovered bugs. Improved leveling method.

#### V1_1.0.3
- 修复了一些已发现bug。改善了调平方式。
- 改善了回原点速度和调平速度。
- 打印过程中，点击【停止】→【确定】 或 打印完成后点击【确定】，会自动保存Z-offset值。为了解决打印过程中调整的Z-offset不会保存问题。
- Fixed some discovered bugs. Improved leveling method.
- Optimize return to origin speed and leveling speed.
- To solve the problem that the Z-offset adjusted during printing will not be saved：During printing, click 【stop】→【confirm】 or click 【confirm】 after printing is completed, the Z-offset value will be saved automatically. 


#### V1_1.0.4（Update in the future）
- 在V1_1.0.3固件基础上改进
- 修复断电续打功能bug：恢复打印时悬空打印。如果断电后Z轴掉落或者模型脱落也会导致续打失败。
- 优化打印操作。打印过程中，点击暂停后挤出机会先回抽10mm，以防止漏料。暂停后恢复会先挤出9mm→回抽9mm→移动到打印位置→继续正常打印，避免了缺料问题
- 
- 
- 
- 

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


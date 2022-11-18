## 说明
- 请根据以下兼容性关系更新主板和屏幕固件。

## 屏幕固件与主板固件兼容关系
## Compatibility between screen firmware and mainboard firmware

| 屏幕固件版本 | 主板固件版本 |
|-------------------------|----------------------------|
| Screen Firmware Version | Mainboard Firmware Version |
|           V1            |  V1_1.0.3  V1_1.0.4  V1_1.0.5-Beta     |



## 主板固件/Board firmware
### V1.0.2
- 增加【调平温度设置】
- 修复了一些已发现bug。改善了调平方式。
- Added 【Leveling Temperature Settings】
- Fixed some discovered bugs. Improved leveling method.

### V1_1.0.3
- 修复了一些已发现bug。改善了调平方式。
- 改善了回原点速度和调平速度。
- 打印过程中，点击【停止】→【确定】 或 打印完成后点击【确定】，会自动保存Z-offset值。为了解决打印过程中调整的Z-offset不会保存问题。
- Fixed some discovered bugs. Improved leveling method.
- Optimize return to origin speed and leveling speed.
- To solve the problem that the Z-offset adjusted during printing will not be saved：During printing, click 【stop】→【confirm】 or click 【confirm】 after printing is completed, the Z-offset value will be saved automatically. 

### V1_1.0.4
- 在固件V1_1.0.3基础上改进。
- 再次优化G28回原点速度和G29调平速度。
- 修改默认调平温度。热端：0℃  / 热床：60℃ 。仍然可以在【设置】中修改调平温度。
- 默认Z-Offset=0.1 。通常情况下，调平后可以直接选择gcode文件打印，无需设置Z-Offset。
- 修复断电续打功能bug：恢复打印时喷嘴距离模型很高。（注意：如果断电后Z轴掉落或者模型脱落也会导致断电续打失败）。
- 优化打印操作。打印过程中，点击【暂停】后挤出机会先回抽10mm，以防止漏料。
- 优化打印操作。暂停打印后，点击【恢复】会先挤出9mm→回抽9mm→移动到打印位置→继续正常打印，避免了换料或暂停打印后出现的缺料问题。
- 优化打印操作。点击【停止】打印后挤出机会回抽10mm，以防止漏料。
- Improved on firmware V1_1.0.3.
- G28 return to the origin speed and G29 leveling speed were optimized again.
- Change the default leveling temperature. Hot end: 0℃ / hot bed: 60℃.You can still change the 【leveling temperature】 in 【Settings】.
- The default value is 【z-offset =0.1】. In general, you do not need to set the z-offset after leveling.
- Fixed a bug in the power-off continuation function where the nozzle distance from the model was too high when recovering print.(Note: If the Z-axis drops or the model falls off after power-off, the power-off refill function cannot be used normally).
- Optimize print operations. In the printing process, after clicking 【Pause】, the extrusion machine will draw back 10mm to prevent material leakage.
- Optimize print operations. After pausing printing, click 【Resume】 to extrude 9mm→ pull back 9mm→ move to the printing position → continue normal printing. Avoid the empty print problem.
- Optimize print operations. After clicking 【Stop】 to print, the extrusion machine will draw back 10mm to prevent material leakage.

### V1_1.0.5-Beta
- 新增Z轴自动补偿倾斜校正功能（实验性）。使用M853 L M控制校正范围( -1 > L > 1 ;  -1 > M > 1 ) 。调试方法：https://github.com/NARUTOfzr/Neptune_3/issues/37
- 新增-可以在gcode中使用M0代码暂停打印，实现指定层暂停功能。使用方法：https://github.com/NARUTOfzr/Neptune_3/issues/38
- 优化打印中的加载/卸载灯丝操作按钮。操作方法：点击卸载/加载灯丝，点击一下，快速卸载/加载灯丝，再点击一下，停止加载灯丝。实现快速换料。操作说明：Instructions for loading/unloading filaments (during printing)
- 启用热床PID。默认值：M304 P97.10 I1.41 D1675.16    调试方法：https://github.com/NARUTOfzr/Neptune_3/issues/39
- 默认关闭断电恢复功能。可以在gcode起始程序中加入M413 S1 开启断电恢复（S0为关闭断电恢复）。
- 启用Host Action Commands。丰富打印机与octoprint联机交互体验。
- 启用线性推进。默认：K= 0 。（参见：https://marlinfw.org/docs/features/lin_advance.html） 
- 修复部分BUG。
- Added Z-axis automatic compensation tilt correction function (experimental). Use M853 L M to control the correction range (-1 > L > 1; -1 > M > 1).       Debugging method: https://github.com/NARUTOfzr/Neptune_3/issues/37
- Added - You can use the M0 code in gcode to pause printing to realize the pause function of the specified layer.      How to use: https://github.com/NARUTOfzr/Neptune_3/issues/38
- Optimized the loading/unloading filament operation button in printing.     Operation method: Click "Unload/Load Filament", click "Quick Unload/Load Filament", and then click "Stop Loading Filament". Realize quick refueling. Operating instructions: Instructions for loading/unloading filaments (during printing)
- Enable Hotbed PID. Default value: M304 P97.10 I1.41 D1675.16     Debug method: https://github.com/NARUTOfzr/Neptune_3/issues/39
- The power failure recovery function is disabled by default. You can add M413 S1 to the gcode start program to turn on power-off recovery (S0 is to turn off power-off recovery).
- Enable Host Action Commands. Enrich the online interactive experience between printer and octoprint.
- Enable linear advance. Default: K=0 .（See：https://marlinfw.org/docs/features/lin_advance.html）
- Fix some bugs.


















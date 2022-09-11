## 说明
- 请根据以下兼容性关系更新主板和屏幕固件。

## 屏幕固件与主板固件兼容关系
## Compatibility between screen firmware and mainboard firmware

| 屏幕固件版本 | 主板固件版本 |
|-------------------------|----------------------------|
| Screen Firmware Version | Mainboard Firmware Version |
|           V1            |        V1.0.2 V1_1.0.3       |



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
- 修复断电续打功能bug：恢复打印时喷嘴距离模型很高。（注意：如果断电后Z轴掉落或者模型脱落也会导致断电续打失败）。
- 优化打印操作。打印过程中，点击【暂停】后挤出机会先回抽10mm，以防止漏料。
- 优化打印操作。暂停打印后，点击【恢复】会先挤出9mm→回抽9mm→移动到打印位置→继续正常打印，避免了换料或暂停打印后出现的缺料问题。
- Improved on firmware V1_1.0.3.
- G28 return to the origin speed and G29 leveling speed were optimized again.
- Fixed a bug in the power-off continuation function where the nozzle distance from the model was too high when recovering print.(Note: If the Z-axis drops or the model falls off after power-off, the power-off refill function cannot be used normally).
- Optimize print operations. In the printing process, after clicking 【Pause】, the extrusion machine will draw back 10mm to prevent material leakage.
- Optimize print operations. After pausing printing, click 【Resume】 to extrude 9mm→ pull back 9mm→ move to the printing position → continue normal printing. Avoid the empty print problem.



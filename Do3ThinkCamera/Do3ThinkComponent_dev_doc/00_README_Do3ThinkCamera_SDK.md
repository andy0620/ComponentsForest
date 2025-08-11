# Do3Think Camera SDK 開發手冊總覽
*Do3Think Industrial Camera SDK Complete Development Manual*

## 📚 文檔架構

本開發手冊為Do3Think工業相機SDK提供了完整的技術文檔，專門針對工業自動化AOI（Automated Optical Inspection）設備開發需求設計。

### 文檔列表

1. **[01_Device_Connection_Initialization.md](01_Device_Connection_Initialization.md)**
   - SDK初始化流程
   - 設備掃描與枚舉
   - 連接管理策略
   - 多相機管理

2. **[02_Parameter_Configuration.md](02_Parameter_Configuration.md)**
   - 相機參數分類
   - 參數讀寫方法
   - 自動模式配置
   - 配置持久化

3. **[03_Image_Acquisition.md](03_Image_Acquisition.md)**
   - 圖像獲取模式
   - 回調機制設計
   - 高速採集優化
   - Zero-copy技術

4. **[04_Error_Handling_Diagnostics.md](04_Error_Handling_Diagnostics.md)**
   - 錯誤碼系統
   - 異常恢復機制
   - 斷線重連策略
   - 診斷工具

5. **[05_Advanced_Features.md](05_Advanced_Features.md)**
   - 觸發模式應用
   - 多相機同步
   - 硬體ISP功能
   - 線掃相機支援

6. **[06_Qt_Integration_Examples.md](06_Qt_Integration_Examples.md)**
   - Qt元件架構
   - Control Panel設計
   - 工業AOI應用範例
   - 性能優化實踐

7. **[07_API_Quick_Reference.md](07_API_Quick_Reference.md)**
   - API分類索引
   - 數據結構參考
   - 錯誤碼對照表
   - 代碼片段庫

---

## 🚀 快速開始指南

### 1. 環境準備
```cpp
// 包含必要頭文件
#include "DVPCamera.h"
#include "dvpParam.h"

// 連結庫文件
#pragma comment(lib, "DVPCamera64.lib")  // 64位系統
#pragma comment(lib, "DVPCamera.lib")    // 32位系統
```

### 2. 基本使用流程
```cpp
// 1. 掃描設備
dvpUint32 count = 0;
dvpRefresh(&count);

// 2. 枚舉設備信息
dvpCameraInfo info;
dvpEnum(0, &info);

// 3. 打開相機
dvpHandle handle;
dvpOpenByName(info.FriendlyName, OPEN_NORMAL, &handle);

// 4. 設置參數
dvpSetFloatValue(handle, V_EXPOSURE_TIME_F, 10000.0);
dvpSetFloatValue(handle, V_GAIN_F, 1.0);

// 5. 開始採集
dvpStart(handle);

// 6. 獲取圖像
dvpFrame frame;
void* pBuffer;
dvpGetFrame(handle, &frame, &pBuffer, 1000);

// 7. 停止並關閉
dvpStop(handle);
dvpClose(handle);
```

---

## 🎯 核心功能特性

### 設備管理
- 支援多相機同時連接
- 熱插拔檢測
- 設備重連機制
- 用戶自定義命名

### 圖像採集
- **同步模式**：阻塞式獲取，簡單可靠
- **異步模式**：回調驅動，高效能
- **觸發模式**：硬體/軟體觸發支援
- **性能優化**：100-1000fps高速採集

### 參數控制
- 曝光時間、增益、白平衡
- ROI設置、圖像格式轉換
- 自動曝光、自動白平衡
- 參數配置保存/載入

### 進階功能
- 多相機同步採集
- 硬體ISP處理
- 線掃相機支援
- 事件回調系統

---

## 💡 開發建議

### 1. 架構設計
- 採用生產者-消費者模式分離採集與處理
- 使用RAII管理相機資源
- 實現自動重連機制確保穩定性

### 2. 性能優化
- 使用回調模式進行高速採集
- 實施Zero-copy減少內存拷貝
- 合理設置緩衝區大小
- 利用多線程並行處理

### 3. 錯誤處理
- 檢查所有API返回值
- 實現完整的錯誤恢復機制
- 建立日誌系統追蹤問題
- 定期健康檢查

### 4. Qt整合
- 繼承BaseComponent實現標準元件
- 使用Signal/Slot進行通訊
- 獨立線程執行圖像採集
- Control Panel與元件解耦

---

## 📊 典型應用場景

### AOI缺陷檢測
```cpp
class AOIInspectionSystem {
    // 高速採集 + 實時處理
    // 觸發模式同步
    // 多相機協同
};
```

### 尺寸測量
```cpp
class DimensionMeasurement {
    // 高精度採集
    // 亞像素處理
    // 標定校準
};
```

### 顏色檢測
```cpp
class ColorInspection {
    // 白平衡控制
    // 顏色空間轉換
    // 色差分析
};
```

---

## 🔧 開發工具

### 必需工具
- Visual Studio 2019/2022
- Qt Creator 10+
- CMake 3.16+

### 調試工具
- BasedCam2（官方調試工具）
- SDK範例程式
- 日誌分析工具

---

## 📈 性能指標

| 指標 | 規格 | 備註 |
|------|------|------|
| 最大幀率 | 1000+ fps | 取決於解析度 |
| 延遲 | <10ms | 回調模式 |
| CPU使用率 | <30% | 單相機 |
| 內存佔用 | <500MB | 含緩衝區 |

---

## 🤝 技術支援

### 文檔資源
- SDK使用指南PDF
- API參考手冊
- 範例程式碼

### 問題排查
1. 檢查驅動安裝
2. 確認USB/GigE連接
3. 查看錯誤碼
4. 分析日誌文件

---

## 📝 版本信息

- **SDK版本**: 2.25.43.1332
- **文檔版本**: 1.0.0
- **更新日期**: 2025年1月
- **支援平台**: Windows 10/11, Linux

---

## 🏗️ 項目結構建議

```
YourProject/
├── components/
│   └── Do3ThinkCamera/
│       ├── Do3ThinkCameraComponent.h
│       ├── Do3ThinkCameraComponent.cpp
│       └── ControlPanel/
├── libs/
│   ├── DVPCamera64.lib
│   └── DVPCamera64.dll
├── config/
│   └── camera_config.json
└── docs/
    └── Do3ThinkComponent_dev_doc/
```

---

## ⚡ 快速問題索引

- **Q: 相機無法連接？**
  → 查看 [01_Device_Connection_Initialization.md](01_Device_Connection_Initialization.md#troubleshooting)

- **Q: 圖像採集失敗？**
  → 查看 [03_Image_Acquisition.md](03_Image_Acquisition.md#common-issues)

- **Q: 參數設置無效？**
  → 查看 [02_Parameter_Configuration.md](02_Parameter_Configuration.md#parameter-dependencies)

- **Q: 性能不達標？**
  → 查看 [06_Qt_Integration_Examples.md](06_Qt_Integration_Examples.md#performance-optimization)

---

## 📌 重要提示

1. **線程安全**：SDK大部分API都是線程安全的，但建議在單一線程中操作同一相機句柄
2. **資源管理**：確保正確釋放資源，避免內存洩漏
3. **錯誤處理**：始終檢查API返回值，實施錯誤恢復機制
4. **性能優化**：合理使用回調模式和緩衝區配置

---

*本開發手冊專為工業自動化AOI設備開發設計，提供從基礎到進階的完整技術指導。*
# ComponentsForest 架構設計

## 🏗️ 三層架構設計

ComponentsForest 採用三層架構設計，提供最大的彈性與可重用性：

```
┌─────────────────────────────────────────────────────────┐
│                    應用層 (Application)                   │
├─────────────────────────────────────────────────────────┤
│                 具體實作層 (Implementation)                │
│   Do3ThinkCameraComponent    Do3ThinkCameraControlPanel   │
│   BaslerCameraComponent      BaslerCameraControlPanel     │
│   HikVisionCameraComponent   HikVisionControlPanel        │
├─────────────────────────────────────────────────────────┤
│                  抽象層 (Abstraction)                     │
│      CameraComponent         CameraControlPanel           │
│      MotionComponent         MotionControlPanel           │
│      AlgorithmComponent      AlgorithmControlPanel        │
├─────────────────────────────────────────────────────────┤
│                    基礎層 (Foundation)                    │
│                      BaseComponent                        │
│                         QObject                           │
└─────────────────────────────────────────────────────────┘
```

## 📋 層級說明

### 1. 基礎層 (Foundation Layer)
**BaseComponent** - 所有元件的基礎類別
- 提供生命週期管理 (initialize, start, stop, destroy)
- 狀態機管理 (State Machine)
- 健康檢查機制 (Health Check)
- 配置管理 (Configuration)
- 事件處理 (Event Handling)
- 效能監控 (Performance Monitoring)

### 2. 抽象層 (Abstraction Layer)
**CameraComponent** - 相機元件的抽象介面
- 定義所有相機的共通操作
- 純虛擬函數：必須實作的核心功能
- 虛擬函數：可選擇覆寫的進階功能
- 標準化的信號/槽介面

**CameraControlPanel** - 相機控制面板的抽象介面
- 定義標準 UI 元件
- 提供基本控制功能
- Signal/Slot 連接框架
- 可擴展的 UI 架構

### 3. 具體實作層 (Implementation Layer)
**Do3ThinkCameraComponent** - Do3Think 相機的具體實作
- 繼承自 CameraComponent
- 實作所有純虛擬函數
- 添加 Do3Think 特定功能
- 處理 SDK 特定邏輯

**Do3ThinkCameraControlPanel** - Do3Think 控制面板
- 繼承自 CameraControlPanel
- 擴展 UI 以支援特定功能
- 保持與元件的完全解耦

## 🔄 繼承關係

```cpp
// 元件繼承鏈
QObject
  └── BaseComponent
      └── CameraComponent
          ├── Do3ThinkCameraComponent
          ├── BaslerCameraComponent
          └── HikVisionCameraComponent

// 控制面板繼承鏈
QWidget
  └── CameraControlPanel
      ├── Do3ThinkCameraControlPanel
      ├── BaslerCameraControlPanel
      └── HikVisionCameraControlPanel
```

## 💡 設計優勢

### 1. **代碼重用性**
- 80% 的功能在抽象層實現
- 只需實作廠商特定的 20% 功能
- 減少重複代碼

### 2. **易於擴展**
- 添加新廠商只需繼承抽象類
- 不影響現有實作
- 符合開閉原則 (OCP)

### 3. **統一介面**
- 所有相機使用相同的介面
- 應用層無需關心具體實作
- 易於切換不同廠商

### 4. **完全解耦**
- 元件與 UI 完全分離
- 僅透過 Signal/Slot 通訊
- 支援遠端控制

### 5. **測試友好**
- 可以 Mock 抽象介面
- 單元測試更容易
- 整合測試更可靠

## 🎯 使用範例

### 使用抽象介面（推薦）
```cpp
// 使用抽象介面，支援任何相機
CameraComponent* camera = CameraFactory::create("Do3Think");
CameraControlPanel* panel = new Do3ThinkCameraControlPanel();
panel->connectToComponent(camera);
```

### 使用具體實作
```cpp
// 直接使用具體實作，存取特定功能
Do3ThinkCameraComponent* camera = new Do3ThinkCameraComponent();
camera->setHDRMode(true);  // Do3Think 特定功能
```

## 📊 類別責任

| 類別 | 責任 | 純虛擬方法 | 虛擬方法 | 具體方法 |
|------|------|-----------|---------|---------|
| BaseComponent | 基礎生命週期 | 0 | 8 | 15 |
| CameraComponent | 相機共通功能 | 12 | 25 | 20 |
| Do3ThinkCameraComponent | Do3Think 實作 | 0 | 37 | 30 |
| CameraControlPanel | UI 共通功能 | 0 | 20 | 25 |
| Do3ThinkCameraControlPanel | Do3Think UI | 0 | 15 | 40 |

## 🔧 擴展指南

### 添加新的相機廠商
1. 創建新類別繼承 `CameraComponent`
2. 實作所有純虛擬方法
3. 添加廠商特定功能
4. 創建對應的 ControlPanel
5. 註冊到 CameraFactory

### 添加新的元件類型
1. 創建抽象類別繼承 `BaseComponent`
2. 定義共通介面
3. 創建具體實作
4. 創建對應的 ControlPanel

## 🚀 效能考量

- **零拷貝**: ImageBuffer 類別支援零拷貝傳輸
- **線程安全**: 每個元件運行在獨立線程
- **異步處理**: Signal/Slot 支援異步通訊
- **緩衝管理**: 環形緩衝區避免記憶體分配

## 📝 設計模式

- **Factory Pattern**: 元件創建
- **Observer Pattern**: Signal/Slot 機制
- **Template Method**: 抽象類別的虛擬方法
- **Strategy Pattern**: 可替換的實作
- **RAII**: 資源管理

## ✅ SOLID 原則

- **S**ingle Responsibility: 每個類別單一職責
- **O**pen/Closed: 對擴展開放，對修改關閉
- **L**iskov Substitution: 子類別可替換父類別
- **I**nterface Segregation: 介面精簡專注
- **D**ependency Inversion: 依賴抽象而非具體

---

*此架構設計確保了 ComponentsForest 的可擴展性、可維護性和可測試性，為工業自動化 AOI 設備提供了堅實的基礎。*
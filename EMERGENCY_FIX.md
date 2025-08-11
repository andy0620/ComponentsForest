# 🚨 緊急修復：應用程式無法啟動

## 問題診斷
從 debug.log 第 18 行可以看到，程式在 QApplication 建構函式停止：
```
[0018] 15:23:17.592 ... Calling QApplication constructor...
```

## 根本原因
**缺少 Qt 平台插件 (qwindows.dll)**

QApplication 無法建立因為找不到 Windows 平台插件。這是 Qt 應用程式部署的常見問題。

## 立即修復方案

### 方法 1：執行修復腳本（推薦）
```batch
FIX_NOW.bat
```
這個腳本會自動：
1. 建立 platforms 資料夾
2. 從 Qt 安裝目錄複製 qwindows.dll
3. 測試應用程式

### 方法 2：手動修復
如果腳本無法運作，請手動執行：

1. **建立 platforms 資料夾**
```cmd
mkdir C:\Users\g4user\Desktop\ComponentsForest\build\viewers\do3think_camera_viewer\Release\platforms
```

2. **複製平台插件**
從您的 Qt 安裝目錄複製：
```
來源: C:\Qt\6.9.1\msvc2022_64\plugins\platforms\qwindows.dll
目標: C:\Users\g4user\Desktop\ComponentsForest\build\viewers\do3think_camera_viewer\Release\platforms\qwindows.dll
```

3. **測試應用程式**
```cmd
cd C:\Users\g4user\Desktop\ComponentsForest\build\viewers\do3think_camera_viewer\Release
Do3ThinkCameraViewerStandalone.exe --minimal
```

## 檔案結構檢查
確保 Release 資料夾有以下結構：
```
Release/
├── Do3ThinkCameraViewerStandalone.exe
├── Qt6Core.dll
├── Qt6Gui.dll
├── Qt6Widgets.dll
├── DVPCamera64.dll
└── platforms/              ← 這個資料夾必須存在
    └── qwindows.dll        ← 這個檔案必須存在
```

## 驗證修復成功
修復後，debug.log 應該顯示：
```
[0019] QApplication constructor returned successfully
[0020] QApplication created successfully
```

## 其他可能需要的插件
如果應用程式使用特定功能，可能還需要：
- `imageformats/` - 圖片格式支援
- `styles/` - 樣式插件
- `iconengines/` - 圖示引擎

## 預防措施
未來部署時，請使用 Qt 的部署工具：
```cmd
windeployqt.exe Do3ThinkCameraViewerStandalone.exe
```

這會自動複製所有必要的 Qt 相依檔案。

---

**立即執行 `FIX_NOW.bat` 來修復問題！**
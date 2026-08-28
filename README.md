# csjplayer

以 FFmpeg(libmpv)為核心引擎的自製 Qt6 影音播放器。永久無邊框視窗、自製標題列,播放清單、A-B 循環、0.1x~32x 正逆向變速播放等功能。

## 功能特色

- 播放/暫停、可點擊與拖曳的進度條
- 播放清單:資料夾遞迴掃描、拖曳加入、增加/移除項目、匯出入 `.m3u` 清單檔
- 三態循環模式:不循環 / 清單循環 / 單集循環 / 播完暫停
- 音量調整、靜音、滑鼠滾輪/鍵盤快捷鍵控制
- 播放速度 0.1x ~ 32x(正向)、0.1x ~ 8x(逆向,原生倒播搭配自動 fallback 模擬倒播)
- A-B 循環片段
- 開檔對話框、拖曳檔案/資料夾到視窗即可播放
- 永久無邊框視窗,自製標題列(拖曳移動、邊緣縮放、最小化/最大化/關閉)
- Alt+1/2/3 視窗尺寸快捷鍵、Ctrl+/ 隱藏所有面板(乾淨播放模式)
- 全螢幕切換(左右鍵同按 / Ctrl+Enter)

## 建置

### 需求

- CMake ≥ 3.21
- C++17 編譯器
- Qt6(Widgets、OpenGLWidgets)
- libmpv(開發套件,含 `mpv/client.h`、`mpv/render.h`)

Arch Linux 上安裝依賴:

```bash
sudo pacman -S cmake qt6-base mpv
```

### 編譯

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

執行檔會在 `build/csjplayer`。直接執行,或帶一個影片路徑當參數:

```bash
./build/csjplayer /路徑/影片.mp4
```

## 安裝

### Arch Linux 套件(PKGBUILD)

```bash
makepkg -si
```

會自動處理相依套件、安裝執行檔、`.desktop` 檔案與圖示。之後可直接在應用程式選單啟動,或在終端機打 `csjplayer`。解除安裝:

```bash
sudo pacman -R csjplayer
```

### Portable AppImage(任何 Linux 發行版)

`dist/` 目錄下的 `.AppImage`(如有建置)是不需安裝、不需系統裝 Qt6/mpv 就能執行的可攜版本:

```bash
chmod +x csjplayer-*.AppImage
./csjplayer-*.AppImage
```

AppImage 預設固定使用 XCB(X11)顯示後端執行,即使在 Wayland 桌面也是透過 XWayland 相容層運作正常;這是刻意的設計,因為打包時的 Qt6 Wayland 平台外掛在不同機器上容易有 ABI 不相容問題。

若要自己重新產生 AppImage,需要 [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy)、[linuxdeploy-plugin-qt](https://github.com/linuxdeploy/linuxdeploy-plugin-qt)、[appimagetool](https://github.com/AppImage/appimagetool),流程大致是:Release 建置 → `DESTDIR=AppDir cmake --install build` → 用 linuxdeploy 的 qt 外掛蒐集依賴函式庫 → 用 appimagetool 封裝。

## 快捷鍵總覽

### 播放控制

| 按鍵 | 動作 |
|---|---|
| Space | 播放/暫停 |
| 雙擊影片畫面 | 播放/暫停 |
| ← / → | 回轉/快轉 1 分鐘 |
| Ctrl+← / Ctrl+→ | 回轉/快轉 5 分鐘 |
| 滑鼠水平滾輪(傾斜) | 同左右鍵 ±1 分鐘;Ctrl+ 則 ±5 分鐘 |
| ↑ / ↓ | 音量 +/− |
| 滑鼠垂直滾輪 | 音量 +/− |
| PageDown / PageUp | 播放清單下一首 / 上一首 |

### A-B 循環

| 按鍵 | 動作 |
|---|---|
| `[` | 設定循環起點(A 點) |
| `]` | 設定循環終點(B 點,設定後立即開始循環) |
| `\` | 取消循環 |

### 視窗

| 按鍵/手勢 | 動作 |
|---|---|
| Alt+1 / Alt+2 / Alt+3 | 視窗尺寸切換為 960×540 / 1920×1080 / 3840×2160 |
| Ctrl+/ | 切換隱藏/顯示標題列與所有控制面板 |
| 滑鼠左鍵拖曳(影片畫面或標題列) | 移動視窗 |
| 滑鼠左鍵拖曳(視窗邊緣/角落) | 縮放視窗 |
| 雙擊標題列 | 最大化/還原 |
| 滑鼠左右鍵同時按下 / Ctrl+Enter | 切換全螢幕 |

### 播放清單面板

| 操作 | 動作 |
|---|---|
| + | 增加檔案(附加到清單) |
| 📁+ | 增加資料夾(遞迴掃描,附加到清單) |
| − / Delete / Backspace | 移除選取的項目 |
| Shift+點選 / Ctrl+點選 | 整排選取 / 多選加選 |
| 雙擊項目 | 播放該項目 |
| 匯出清單 / 匯入清單 | 存成或讀取 `.m3u` 播放清單檔 |

### 播放速度

底部控制列的方向鈕(▶/◀)切換正向/倒轉播放,下拉選單選擇倍速(0.1x~32x,倒轉時限制在 8x 以內)。倒轉播放優先使用 mpv 原生倒播,若該檔案/編碼不支援會自動切換為模擬倒播(此模式無聲音)。切換播放清單裡的檔案時,會自動重置回正向 1x。

## 已知限制

- **四角吸附未實作**:Wayland 協定不允許應用程式自行設定視窗絕對位置(不論是拖曳中或程式化呼叫),此功能在 Wayland 桌面環境下無法實現,已放棄嘗試。
- **視窗永遠置頂未實作**:同樣是 Wayland 的限制,標準協定沒有讓應用程式要求「永遠在最上層」的機制。多數桌面環境(如 GNOME)有自己的視窗選單可以達成這個效果(嘗試按 Alt+Space 或 Super+Space 開啟視窗操作選單)。
- 模擬倒播模式下沒有聲音(mpv 沒有音訊倒放的機制)。

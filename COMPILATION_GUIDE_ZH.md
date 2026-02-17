# AyuGramDesktop 編譯與維護指南（繁中）

最後更新：2026-02-17

## 1. 目標
本 fork 目前以 Windows x64（VS2022）為主，重點是：
- AyuGram 介面與設定本地化（繁中）
- 訊息右鍵功能增強（Details / 查看 JSON / 選取）
- JSON Viewer 強化（樹狀可摺疊）
- 群組成員 Profile 顯示 Joined 時間（含多層 fallback + 快取）

## 2. 環境需求
- Windows 10/11
- Visual Studio 2022（Desktop development with C++）
- CMake（建議 3.24+）
- Git

## 3. 一次性依賴建置
根目錄執行：
```bat
run_configure.bat
```
說明：
- 這一步通常最久（可能數小時）
- 只要第三方依賴與 CMake cache 未重大變更，通常不需每次重跑

## 4. 日常編譯
根目錄執行：
```bat
run_build.bat
```
輸出：
- `out/Release/AyuGram.exe`

建議：
- 編譯前先關閉正在執行的 AyuGram.exe
- 若出現大量 `msbuild` / `conhost` 殘留，先清理進程後再重編

## 5. 常見問題
- `PEER_ID_INVALID`：不一定是 Joined 功能本身；可能是其他 RPC。當前版本已加入 fallback 與快取回填。
- Joined 偶發 `Unknown`：多為 Telegram 回傳 participant `date=0`（資料端未提供），非單純程式錯誤。
- 右鍵項目位置異常：優先檢查 `history_view_context_menu.cpp` 與 `ayu/ui/context_menu/context_menu.cpp`。

## 6. 回歸測試清單
每次更新後至少驗證：
1. 訊息右鍵可見 `Details` / `查看 JSON`，且位置在底部區段
2. `查看 JSON` 可開啟樹狀檢視、可展開/收合、可複製 JSON
3. reply / quote 背景色正常
4. 群組管理員在群內開成員 profile，Joined 顯示正常（日期或 Unknown）
5. build 成功且程式可啟動

## 7. 下次同步 upstream 前準備
建議流程：
1. 先提交目前 fork 變更（保持工作樹乾淨）
2. 拉取 upstream 最新
3. 優先解衝突檔案：
   - `info/profile/info_profile_actions.cpp`
   - `boxes/peers/prepare_short_info_box.cpp`
   - `history/view/history_view_context_menu.cpp`
   - `ayu/*` 自訂檔
4. 重跑 `run_build.bat`
5. 依「回歸測試清單」全測一次

## 8. 建議提交策略
- 功能分支拆分：
  - `feature/json-viewer`
  - `feature/profile-joined-time`
  - `feature/context-menu-enhancements`
- 每個分支保持單一主題，降低 rebase 成本

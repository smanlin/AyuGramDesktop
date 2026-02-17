# MISSION SUMMARY（Fork 現況）

最後更新：2026-02-17

## 一、已完成主線

### 1) 訊息右鍵與 Details 流程
- 恢復/優化訊息右鍵選單
- `Details` 與 `查看 JSON` 已調整到選單底部區段
- 避免 `查看 JSON` 作為子選單

關鍵檔案：
- `Telegram/SourceFiles/history/view/history_view_context_menu.cpp`
- `Telegram/SourceFiles/ayu/ui/context_menu/context_menu.cpp`

### 2) JSON Viewer 重構
- 新增可摺疊節點的樹狀 JSON 檢視
- 修正 UI 白塊、字體與可讀性問題
- 保留複製 JSON 功能

關鍵檔案：
- `Telegram/SourceFiles/ayu/ui/boxes/json_viewer_box.cpp`
- `Telegram/SourceFiles/ayu/ui/boxes/json_viewer_box.h`

### 3) Reply / Quote 視覺回歸
- 修復回覆與引用訊息背景色遺失問題

關鍵檔案：
- `Telegram/SourceFiles/history/view/history_view_bottom_info.cpp`
- `Telegram/SourceFiles/history/view/history_view_bottom_info.h`
- `Telegram/SourceFiles/ui/chat/chat_style.cpp`

### 4) 群組成員 Joined 顯示（Profile）
- 在群組管理上下文的成員 profile 中顯示 Joined
- 多層查詢策略：
  - `channels.getParticipant`
  - `PEER_ID_INVALID` 時 refresh participants（Recent / Search(username)）
  - 多候選群組切換重試
- 狀態語義：
  - `Loading...` / `Unknown` / `Unavailable`
- 新增持久化快取（`channelId:userId -> joined_time`），跨重啟可回填

關鍵檔案：
- `Telegram/SourceFiles/info/profile/info_profile_actions.cpp`
- `Telegram/SourceFiles/ayu/ayu_settings.h`
- `Telegram/SourceFiles/ayu/ayu_settings.cpp`

## 二、目前已知限制
- Telegram 回傳 participant `date=0` 時只能標示 `Unknown`
- `PEER_ID_INVALID` 可能來自其他 RPC，不一定是 Joined 查詢

## 三、下次更新 upstream 的實作建議
1. 先保持工作樹乾淨（功能先 commit）
2. 先解 `info_profile_actions.cpp` 衝突，再解 `ayu/*` 與 context menu
3. 重新編譯並跑完整回歸（右鍵、JSON、Joined、背景色）
4. 若 Joined 回歸退化，優先檢查：
   - participant 類型解析
   - fallback 分支是否還在
   - Ayu settings 的 joined cache 欄位是否保留

## 四、交付標準（維護基線）
- Release 可編譯
- 核心功能可用：
  - 右鍵選取/Details/查看 JSON
  - JSON 樹狀檢視
  - Joined 顯示（至少 Loading/Unknown 不崩）
  - reply/quote 背景正常

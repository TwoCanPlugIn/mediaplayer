// fusion_browse.hpp
//
// Fusion head-unit side of the USB/MTP folder-browse protocol, derived from candump captures.
//
//   Head unit -> player : PGN 126720, SA 33 -> DA 15   (fast-packet)
//   Player -> head unit : PGN 130820, SA 15, broadcast (fast-packet)
//
// All payloads below are the *reassembled* fast-packet payload, starting at 0xA3 0x99.
//
// Common header:   [0]=0xA3 [1]=0x99 [2]=command [3]=0x00 (HU) / 0x80 (player) [4]=sessionId
//   sessionId = the source ID. Read it from the player's 0x02 source-list replies; it was
//   USB=0x05, MTP=0x07 in the captures, but don't hard-code it.
//   lockId    = one byte chosen by the head unit, used on every browse message, echoed by the
//               player in 0x0F / 0x10 / 0x11. Ignore 0x0F/0x10/0x11 carrying another lockId.
//
// ---------------------------------------------------------------------------------------------
// SEQUENCE: list current folder / enter folder / go up
//
//   HU  09  folderId, op          op: 01 = list current, 02 = select item folderId, 03 = up
//   P   0F  idx, ackOp            3-100 ms later.  ackOp 02 (or 01 at root) = a list is ready
//                                                  ackOp 04 = a FILE was selected, now playing
//       ... 300 ms ...
//   HU  0A                        how many items?
//   P   10  n
//       ... 300 ms ...
//   HU  0B  start=0, count=n
//   P   11  x n                   one per item: idx, attr (4F/47 folder, 1F file), name
//
// SEQUENCE: play a track  (op 02 on a file item)
//
//   HU  09  folderId=<file idx>, op=02
//   P   0F  ackOp=04              no 0A/0B follows
//   P   04 / 05 / 06 / 07 ...     pushed repeatedly while tags load (state, title, artist, album)
//   P   09 ...                    position in ms, every ~0.5 s
//
// SEQUENCE: transport
//
//   HU  03  code                  01 play, 02 pause, 04 next, 06 previous
//   P   04 (+05..07 on track change)
// ---------------------------------------------------------------------------------------------

#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace fusion {

// ----- commands, head unit -> player ---------------------------------------------------------
constexpr uint8_t CMD_TRANSPORT     = 0x03;
constexpr uint8_t CMD_BROWSE_ACTION = 0x09;
constexpr uint8_t CMD_COUNT_REQ     = 0x0A;
constexpr uint8_t CMD_ITEMS_REQ     = 0x0B;

// ----- commands, player -> head unit ---------------------------------------------------------
constexpr uint8_t RSP_STATE      = 0x04;   // play state, track no, total, duration, position
constexpr uint8_t RSP_TITLE      = 0x05;
constexpr uint8_t RSP_ARTIST     = 0x06;
constexpr uint8_t RSP_ALBUM      = 0x07;
constexpr uint8_t RSP_POSITION   = 0x09;   // u32 ms
constexpr uint8_t RSP_BROWSE_ACK = 0x0F;
constexpr uint8_t RSP_COUNT      = 0x10;
constexpr uint8_t RSP_ITEM       = 0x11;
constexpr uint8_t RSP_HEARTBEAT  = 0x20;

// ----- browse ops (HU 0x09) ------------------------------------------------------------------
constexpr uint8_t OP_LIST_CURRENT = 0x01;  // what your code calls ROOT_FOLDER
constexpr uint8_t OP_SELECT       = 0x02;  // enter folder / play file at folderId
constexpr uint8_t OP_UP           = 0x03;  // parent folder

// ----- ack ops (player 0x0F) -----------------------------------------------------------------
constexpr uint8_t ACK_LIST_ROOT = 0x01;    // seen only for the root listing
constexpr uint8_t ACK_LIST      = 0x02;    // a folder listing is ready
constexpr uint8_t ACK_PLAYING   = 0x04;    // a file was selected

// ----- transport codes (HU 0x03) -------------------------------------------------------------
constexpr uint8_t TRANSPORT_PLAY  = 0x01;
constexpr uint8_t TRANSPORT_PAUSE = 0x02;
constexpr uint8_t TRANSPORT_NEXT  = 0x04;
constexpr uint8_t TRANSPORT_PREV  = 0x06;

constexpr uint32_t HU_STEP_DELAY_MS = 300;   // spacing the real HU uses between 09 -> 0A -> 0B
constexpr uint32_t REPLY_TIMEOUT_MS = 1500;

// =============================================================================================
// Request builders (PGN 126720 payloads)
// =============================================================================================

inline void push_u32(std::vector<uint8_t>& p, uint32_t v) {
    p.push_back(v & 0xFF);
    p.push_back((v >> 8) & 0xFF);
    p.push_back((v >> 16) & 0xFF);
    p.push_back((v >> 24) & 0xFF);
}

// Your original example, generalised over the op byte.
inline std::vector<uint8_t> buildBrowseAction(uint8_t sessionId, uint32_t folderId,
                                              uint8_t op, uint8_t lockId) {
    std::vector<uint8_t> payload;
    payload.push_back(0xA3);
    payload.push_back(0x99);
    payload.push_back(CMD_BROWSE_ACTION);
    payload.push_back(0x00);
    payload.push_back(sessionId);
    payload.push_back(folderId & 0xFF);
    payload.push_back((folderId >> 8) & 0xFF);
    payload.push_back((folderId >> 16) & 0xFF);
    payload.push_back((folderId >> 24) & 0xFF);
    payload.push_back(op);            // was ROOT_FOLDER
    payload.push_back(lockId);
    return payload;
}

inline std::vector<uint8_t> buildCountRequest(uint8_t sessionId, uint8_t lockId) {
    std::vector<uint8_t> payload;
    payload.push_back(0xA3);
    payload.push_back(0x99);
    payload.push_back(CMD_COUNT_REQ);
    payload.push_back(0x00);
    payload.push_back(sessionId);
    payload.push_back(lockId);
    return payload;
}

inline std::vector<uint8_t> buildItemsRequest(uint8_t sessionId, uint32_t start,
                                              uint32_t count, uint8_t lockId) {
    std::vector<uint8_t> payload;
    payload.push_back(0xA3);
    payload.push_back(0x99);
    payload.push_back(CMD_ITEMS_REQ);
    payload.push_back(0x00);
    payload.push_back(sessionId);
    push_u32(payload, start);
    push_u32(payload, count);
    payload.push_back(lockId);
    return payload;
}

inline std::vector<uint8_t> buildTransport(uint8_t sessionId, uint8_t code) {
    std::vector<uint8_t> payload;
    payload.push_back(0xA3);
    payload.push_back(0x99);
    payload.push_back(CMD_TRANSPORT);
    payload.push_back(0x00);
    payload.push_back(sessionId);
    payload.push_back(code);
    return payload;
}

// =============================================================================================
// Browse client
// =============================================================================================

struct Item {
    uint32_t    idx  = 0;
    uint8_t     attr = 0;       // 0x4F / 0x47 folder, 0x1F file (bit meanings otherwise unknown)
    std::string name;
    bool isFolder() const { return (attr & 0x40) != 0; }
};

struct NowPlaying {
    uint16_t    state      = 0;   // 0 = initial, 1 = playing, 2 = paused
    uint32_t    track      = 0;   // 1-based, within the current folder
    uint32_t    total      = 0;   // files in that folder
    uint32_t    durationMs = 0;   // 0 until the player has loaded the file
    uint32_t    positionMs = 0;   // unreliable in this message; use RSP_POSITION instead
    std::string title, artist, album;
};

class BrowseClient {
public:
    using SendFn = std::function<void(const std::vector<uint8_t>&)>;   // send on PGN 126720 -> DA 15

    // A completed listing. cursor = player-reported idx from the ack (after OP_UP this is the
    // position of the folder you just left, within this list).
    std::function<void(const std::vector<Item>& items, uint32_t cursor)> onList;
    // A file was selected and is playing (ack op 04).
    std::function<void()> onPlayStarted;
    std::function<void(const NowPlaying&)> onNowPlaying;
    std::function<void(uint32_t ms)>       onPosition;
    std::function<void(const char* what)>  onTimeout;

    BrowseClient(SendFn send, uint8_t sessionId, uint8_t lockId)
        : send_(std::move(send)), sessionId_(sessionId), lockId_(lockId) {}

    // --- user actions (one browse operation in flight at a time) -----------------------------
    bool listCurrent()         { return begin(0, OP_LIST_CURRENT); }
    bool enter(uint32_t idx)   { return begin(idx, OP_SELECT); }     // folder -> list, file -> plays
    bool up()                  { return begin(cursor_, OP_UP); }
    void transport(uint8_t c)  { send_(buildTransport(sessionId_, c)); }
    bool busy() const          { return step_ != Step::Idle; }

    const std::vector<Item>& items() const { return items_; }

    // Call regularly (e.g. every 10-20 ms) with a monotonic millisecond clock.
    void tick(uint32_t nowMs) {
        switch (step_) {
        case Step::SendCountDue:
            if (nowMs >= due_) {
                send_(buildCountRequest(sessionId_, lockId_));
                lastReq_ = nowMs; step_ = Step::WaitCount;
            }
            break;
        case Step::SendItemsDue:
            if (nowMs >= due_) {
                send_(buildItemsRequest(sessionId_, 0, expected_, lockId_));
                lastReq_ = nowMs; step_ = Step::WaitItems;
            }
            break;
        case Step::WaitAck: case Step::WaitCount: case Step::WaitItems:
            if (nowMs - lastReq_ > REPLY_TIMEOUT_MS) {
                step_ = Step::Idle;
                if (onTimeout) onTimeout("no reply from player");
            }
            break;
        default: break;
        }
        now_ = nowMs;
    }

    // Feed every reassembled PGN 130820 payload from the player (SA 15) here.
    void onPlayerMessage(const uint8_t* b, size_t len, uint32_t nowMs) {
        now_ = nowMs;
        if (len < 5 || b[0] != 0xA3 || b[1] != 0x99 || b[3] != 0x80) return;
        if (b[4] != sessionId_) return;                       // other source
        switch (b[2]) {
        case RSP_BROWSE_ACK: onAck(b, len);    break;
        case RSP_COUNT:      onCount(b, len);  break;
        case RSP_ITEM:       onItem(b, len);   break;
        case RSP_STATE:      onState(b, len);  break;
        case RSP_TITLE:      onString(b, len, np_.title);  break;
        case RSP_ARTIST:     onString(b, len, np_.artist); break;
        case RSP_ALBUM:      onString(b, len, np_.album);  break;
        case RSP_POSITION:
            if (len >= 9 && onPosition) onPosition(rd32(b + 5));
            break;
        default: break;                                       // 0x20 heartbeat, etc.
        }
    }

private:
    enum class Step { Idle, WaitAck, SendCountDue, WaitCount, SendItemsDue, WaitItems };

    static uint32_t rd32(const uint8_t* p) {
        return p[0] | (p[1] << 8) | (p[2] << 16) | (uint32_t(p[3]) << 24);
    }

    bool begin(uint32_t folderId, uint8_t op) {
        if (step_ != Step::Idle) return false;
        send_(buildBrowseAction(sessionId_, folderId, op, lockId_));
        lastReq_ = now_; step_ = Step::WaitAck;
        return true;
    }

    // 0F: [4]=src [5..8]=idx [9]=ackOp [10]=lockId
    void onAck(const uint8_t* b, size_t len) {
        if (len < 11 || b[10] != lockId_ || step_ != Step::WaitAck) return;
        cursor_ = rd32(b + 5);
        uint8_t ackOp = b[9];
        if (ackOp == ACK_PLAYING) {
            step_ = Step::Idle;
            if (onPlayStarted) onPlayStarted();
        } else {                                              // ACK_LIST or ACK_LIST_ROOT
            step_ = Step::SendCountDue;
            due_ = lastReq_ + HU_STEP_DELAY_MS;
        }
    }

    // 10: [4]=src [5..8]=n [9]=lockId
    void onCount(const uint8_t* b, size_t len) {
        if (len < 10 || b[9] != lockId_ || step_ != Step::WaitCount) return;
        expected_ = rd32(b + 5);
        items_.clear();
        if (expected_ == 0) {
            step_ = Step::Idle;
            if (onList) onList(items_, cursor_);
            return;
        }
        step_ = Step::SendItemsDue;
        due_ = lastReq_ + HU_STEP_DELAY_MS;
    }

    // 11: [4]=src [5..8]=idx [9]=attr [10]=lockId [11]=nameLen [12..]=name, NUL
    void onItem(const uint8_t* b, size_t len) {
        if (len < 12 || b[10] != lockId_ || step_ != Step::WaitItems) return;
        size_t nameLen = b[11];
        if (len < 12 + nameLen) return;
        Item it;
        it.idx  = rd32(b + 5);
        it.attr = b[9];
        it.name.assign(reinterpret_cast<const char*>(b + 12), nameLen);
        items_.push_back(std::move(it));
        if (items_.size() >= expected_) {
            step_ = Step::Idle;
            if (onList) onList(items_, cursor_);
        }
    }

    // 04: [4]=src [5..6]=state [7..10]=track [11..14]=total [15..18]=duration [19..22]=position
    void onState(const uint8_t* b, size_t len) {
        if (len < 23) return;
        np_.state      = b[5] | (b[6] << 8);
        np_.track      = rd32(b + 7);
        np_.total      = rd32(b + 11);
        np_.durationMs = rd32(b + 15);
        np_.positionMs = rd32(b + 19);
        if (onNowPlaying) onNowPlaying(np_);
    }

    // 05/06/07: [4]=src [5..8]=1 [9]=len [10..]=string, NUL. A lone " " means "not loaded yet".
    void onString(const uint8_t* b, size_t len, std::string& dst) {
        if (len < 10) return;
        size_t n = b[9];
        if (len < 10 + n) return;
        dst.assign(reinterpret_cast<const char*>(b + 10), n);
        if (dst == " ") dst.clear();
        if (onNowPlaying) onNowPlaying(np_);
    }

    SendFn   send_;
    uint8_t  sessionId_, lockId_;
    Step     step_ = Step::Idle;
    uint32_t now_ = 0, lastReq_ = 0, due_ = 0, expected_ = 0, cursor_ = 0;
    std::vector<Item> items_;
    NowPlaying np_;
};

} // namespace fusion

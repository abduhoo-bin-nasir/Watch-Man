// Ui.cpp - everything drawn on top of the 3D view: HUD, dialogue box, journal (case / notes / people / board), ending.
// It only draws; all decisions are made in Game.cpp.
#include "Ui.h"
#include "Game.h"

namespace {
const Color INK{28, 34, 70, 255}, RED_INK{168, 40, 36, 255}, TYPED{44, 40, 36, 255}, PAPER_DIM{110, 96, 70, 255};
const Color LEATHER{64, 40, 28, 255}, AMBER{232, 176, 74, 255};

Texture2D makeNoiseTex(int w, int h, Color base, int var, uint32_t seed, int specks, Color speck) {
    Image im = GenImageColor(w, h, base);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int n = ((int)(hash32((uint32_t)x, (uint32_t)y, seed) & 255) - 128) * var / 128;
            Color c{(unsigned char)std::clamp(base.r + n, 0, 255), (unsigned char)std::clamp(base.g + n, 0, 255), (unsigned char)std::clamp(base.b + n, 0, 255), 255};
            ImageDrawPixel(&im, x, y, c);
        }
    for (int i = 0; i < specks; i++) ImageDrawCircle(&im, (int)(hash32((uint32_t)i, 1, seed) % (uint32_t)w), (int)(hash32((uint32_t)i, 2, seed) % (uint32_t)h), 1 + (int)(hash32((uint32_t)i, 3, seed) % 2u), speck);
    Texture2D t = LoadTextureFromImage(im);
    UnloadImage(im);
    return t;
}
void tileFill(Texture2D t, Rectangle r, float sc, Color tint) {
    BeginScissorMode((int)r.x, (int)r.y, (int)r.width, (int)r.height);
    float tw = t.width * sc, th = t.height * sc;
    for (float y = r.y; y < r.y + r.height; y += th)
        for (float x = r.x; x < r.x + r.width; x += tw) DrawTextureEx(t, Vector2{x, y}, 0, sc, tint);
    EndScissorMode();
}

float mt(const string& s, int fs) { return MeasureTextEx(GetFontDefault(), s.c_str(), (float)fs, fs / 10.0f).x; }
vector<string> wrapText(const string& s, int fs, float maxW) {
    vector<string> out; string line, word;
    auto push = [&]() {
        if (word.empty()) return;
        string t = line.empty() ? word : line + " " + word;
        if (!line.empty() && mt(t, fs) > maxW) { out.push_back(line); line = word; } else line = t;
        word.clear();
    };
    for (char ch : s) { if (ch == ' ') push(); else word += ch; }
    push();
    if (!line.empty()) out.push_back(line);
    if (out.empty()) out.push_back("");
    return out;
}
void typed(const string& s, float x, float y, int fs, Color c) { DrawTextPro(GetFontDefault(), s.c_str(), Vector2{x, y}, Vector2{0, 0}, 0, (float)fs, fs / 10.0f, c); }
void hand(const string& s, float x, float y, int fs, Color c, int seed = 0) {      // handwriting: a slightly different tilt and baseline per line
    uint32_t h = hash32((uint32_t)seed, (uint32_t)s.size(), 77);
    float rot = ((int)(h & 255) / 255.0f - 0.5f) * 1.4f, dy = ((int)((h >> 8) & 255) / 255.0f - 0.5f) * 2.0f;
    DrawTextPro(GetFontDefault(), s.c_str(), Vector2{x, y + dy}, Vector2{0, 0}, rot, (float)fs, fs / 10.0f, c);
}
void scribble(float x0, float x1, float y, int seed, Color c, float th = 1.5f) {  // a hand-drawn underline
    Vector2 prev{x0, y};
    for (float x = x0 + 8; x < x1 + 8; x += 8) {
        float xx = std::min(x, x1), yy = y + ((int)(hash32((uint32_t)(x * 3), (uint32_t)seed, 5) & 255) / 255.0f - 0.5f) * 2.6f;
        DrawLineEx(prev, Vector2{xx, yy}, th, c); prev = Vector2{xx, yy};
    }
}
void stamp(const string& t, float cx, float cy, int fs, Color c, float deg) {
    float tw = mt(t, fs), th = (float)fs, pw = 10, ph = 6, a = deg * 0.0174533f, ca = std::cos(a), sa = std::sin(a);
    auto P = [&](float dx, float dy) { return Vector2{cx + dx * ca - dy * sa, cy + dx * sa + dy * ca}; };
    float hw = tw / 2 + pw, hh = th / 2 + ph;
    Vector2 q[4] = {P(-hw, -hh), P(hw, -hh), P(hw, hh), P(-hw, hh)};
    for (int i = 0; i < 4; i++) DrawLineEx(q[i], q[(i + 1) % 4], 3.0f, c);
    DrawTextPro(GetFontDefault(), t.c_str(), P(-tw / 2, -th / 2), Vector2{0, 0}, deg, (float)fs, fs / 10.0f, c);
}
string fmtTime(int m) { char b[16]; std::snprintf(b, sizeof b, "%02d:%02d", (m / 60) % 24, m % 60); return b; }

// a pencil sketch of a person, taken from their sprite
void sketch(const Character& n, float x, float y, float h) {
    const SpriteArt& a = spriteSet(n.art).front;
    float sc = std::min(3.0f, h / (float)a.h), w = a.w * sc;
    float oy = y + h - a.h * sc;
    for (int py = 0; py < a.h; py++)
        for (int px = 0; px < a.w; px++) {
            size_t i = (size_t)py * a.w + px; char ch = a.c[i]; RGB col;
            if (ch == ' ' || !spriteColor(ch, n.main, n.acc, col)) continue;
            float lum = ch == 'O' ? 0.05f : (0.3f * col.r + 0.59f * col.g + 0.11f * col.b) / 255.0f;
            if (a.s[i] < 0) lum *= 0.7f;
            int al = (int)std::clamp(20.0f + 190.0f * (1.0f - lum), 20.0f, 255.0f);
            DrawRectangleV(Vector2{x + (40 * 0) + px * sc + (3.0f * 28 - w) * 0.0f, oy + py * sc}, Vector2{std::ceil(sc), std::ceil(sc)}, Color{50, 38, 30, (unsigned char)al});
        }
}

struct Block { int rows; std::function<void(float x, float y, float w)> draw; };
// lay blocks out page after page (left, right, left, right...) and draw the ones that sit on the current spread
int flowBlocks(const vector<Block>& bl, int spread, Rectangle L, Rectangle R, float topOff, float lh, float mx, float mw) {
    int rowsPer = std::max(3, (int)((L.height - topOff - lh * 1.3f) / lh)), slot = 0, row = 0, maxSlot = 0;
    for (const Block& b : bl) {
        int rr = std::min(b.rows, rowsPer);
        if (row > 0 && row + rr > rowsPer) { slot++; row = 0; }
        if (slot / 2 == spread) { const Rectangle& pg = (slot % 2 == 0) ? L : R; b.draw(pg.x + mx, pg.y + topOff + row * lh, mw); }
        row += rr; maxSlot = std::max(maxSlot, slot);
    }
    return maxSlot / 2 + 1;
}

}  // namespace

void Ui::drawJournal(Game& g, int ww, int wh) {
    DrawRectangle(0, 0, ww, wh, Color{6, 5, 4, 218});
    float bh = std::min((float)wh - 100.0f, ((float)ww - 60.0f) / 1.5f), bw = bh * 1.5f;
    float bx = (ww - bw) / 2, by = (wh - bh) / 2 + 6, k = bh / 680.0f;
    int fs = std::max(14, (int)(16 * k + 0.5f)); float lh = std::floor(fs * 1.55f);
    float pad = std::max(10.0f, 14 * k), mx = 46 * k + 8, mw = bw / 2 - pad - mx - 22 * k - 6;
    Rectangle L{bx + pad, by + pad, bw / 2 - pad, bh - 2 * pad}, R{bx + bw / 2, by + pad, bw / 2 - pad, bh - 2 * pad};
    float topOff = lh * 1.7f;

    // cover, shadow, stitching
    DrawRectangleRounded(Rectangle{bx + 8, by + 10, bw, bh}, 0.02f, 6, Color{0, 0, 0, 130});
    DrawRectangleRounded(Rectangle{bx, by, bw, bh}, 0.025f, 8, LEATHER);
    DrawRectangleRoundedLines(Rectangle{bx + 3, by + 3, bw - 6, bh - 6}, 0.025f, 8, Color{40, 24, 16, 255});
    for (float x = bx + 8; x < bx + bw - 8; x += 12) { DrawRectangle((int)x, (int)(by + 6), 6, 2, Color{120, 88, 60, 200}); DrawRectangle((int)x, (int)(by + bh - 8), 6, 2, Color{120, 88, 60, 200}); }

    bool board = g.journal.tab == 3;
    int total = 1;
    if (!board) {
        for (const Rectangle& pg : {L, R}) {
            tileFill(paperTex, pg, 2.0f, Color{240, 226, 190, 255});
            for (float y = pg.y + topOff + lh - 4; y < pg.y + pg.height - lh; y += lh) DrawLine((int)pg.x + 6, (int)y, (int)(pg.x + pg.width - 8), (int)y, Color{110, 130, 160, 60});
            DrawLine((int)(pg.x + 38 * k + 6), (int)pg.y, (int)(pg.x + 38 * k + 6), (int)(pg.y + pg.height), Color{190, 80, 70, 120});
        }
        float g = 46 * k + 10;                                                  // shadow where the pages curve into the spine
        DrawRectangleGradientH((int)(L.x + L.width - g), (int)L.y, (int)g, (int)L.height, Color{60, 40, 20, 0}, Color{50, 30, 14, 150});
        DrawRectangleGradientH((int)R.x, (int)R.y, (int)g, (int)R.height, Color{50, 30, 14, 150}, Color{60, 40, 20, 0});
        for (int i = 0; i < 4; i++) { DrawLine((int)(R.x + R.width + i * 2), (int)(R.y + 4 + i), (int)(R.x + R.width + i * 2), (int)(R.y + R.height - 4 - i), Color{200, 186, 150, 200}); }
        DrawRectangle((int)(bx + bw / 2 - 1), (int)by + 4, 2, (int)bh - 8, Color{30, 18, 10, 255});
        DrawRectangle((int)(bx + bw / 2 + 22 * k), (int)(by + bh - pad), (int)(7 * k + 3), (int)(34 * k + 12), Color{150, 34, 30, 255});   // ribbon marker
    }

    // tabs along the top edge
    const char* TN[4] = {"CASE", "NOTES", "PEOPLE", "BOARD"};
    const Color TC[4] = {Color{92, 80, 60, 255}, Color{120, 66, 44, 255}, Color{64, 84, 90, 255}, Color{104, 40, 38, 255}};
    float tx = bx + 28;
    for (int i = 0; i < 4; i++) {
        float tw = mt(TN[i], fs) + 28, th = i == g.journal.tab ? 30.0f : 24.0f;
        tabRects[i] = Rectangle{tx, by - th + 2, tw, th};
        DrawRectangleRounded(tabRects[i], 0.3f, 4, i == g.journal.tab ? TC[i] : Color{(unsigned char)(TC[i].r * 0.6f), (unsigned char)(TC[i].g * 0.6f), (unsigned char)(TC[i].b * 0.6f), 255});
        typed(TN[i], tx + 14, by - th + (th - fs) / 2 + 3, fs, i == g.journal.tab ? Color{240, 226, 190, 255} : Color{170, 156, 124, 255});
        tx += tw + 6;
    }

    // ------------------------------------------------------------ CASE
    if (g.journal.tab == 0) {
        float x0 = L.x + mx, y = L.y + lh * 0.5f;
        typed("PRECINCT 7 - HOMICIDE", x0, y, fs + 2, TYPED); typed("PRECINCT 7 - HOMICIDE", x0 + 1, y, fs + 2, TYPED);
        typed("CASE FILE 30-1112", x0, y + lh, fs, PAPER_DIM);
        DrawLine((int)x0, (int)(y + lh * 2.1f), (int)(x0 + mw), (int)(y + lh * 2.1f), Color{60, 50, 40, 180});
        const char* F[8][2] = {{"VICTIM", "Anton Reznikov, 41"}, {"LOCATION", "Block 7, apt 106"}, {"REPORTED", "Tue 12 Nov 2030, 07:15"}, {"T.O.D.", "approx. 02:00 - 02:30"},
                               {"CAUSE", "blunt force trauma"}, {"SCENE", "SEALED. Forensics en route."}, {"ASSIGNED", "Det. (you)"}, {"STATUS", g.solved ? "CLOSED" : "OPEN"}};
        float col = mt("REPORTED", fs) + 14, yy = y + lh * 2.5f;
        for (auto& f : F) { typed(f[0], x0, yy, fs, PAPER_DIM); typed(f[1], x0 + col, yy, fs, TYPED); yy += lh; }
        stamp(g.solved ? "CLOSED" : "OPEN", x0 + mw - 60 * k, yy - lh * 0.55f, fs + 6, g.solved ? Color{40, 110, 60, 220} : Color{168, 40, 36, 200}, -9);
        yy += lh * 0.8f;
        const char* note = "Dept. gave me a talking watch. It stopped talking to me on the stairs. Swears like a docker. Fix takes a week. Pen and paper do not sulk.";
        int i = 0;
        for (const string& ln : wrapText(note, fs, mw)) { hand(ln, x0, yy + i * lh, fs, INK, 10 + i); i++; }

        // right page: leads
        float rx = R.x + mx, ry = R.y + lh * 0.3f;
        hand("OPEN LEADS", rx, ry, fs + 4, RED_INK, 1); scribble(rx, rx + mt("OPEN LEADS", fs + 4), ry + fs + 6, 3, RED_INK);
        float cy = R.y + topOff; int hidden = 0;
        for (const string& id : g.book.topo) {
            const Clue& c = g.book.clue(id);
            if (cy > R.y + R.height - lh * 4.5f) break;
            if (c.found) {
                DrawRectangleLines((int)rx, (int)(cy + 2), fs - 3, fs - 3, INK);
                DrawLineEx(Vector2{rx + 1, cy + fs * 0.5f}, Vector2{rx + fs * 0.4f, cy + fs - 2}, 2, INK); DrawLineEx(Vector2{rx + fs * 0.4f, cy + fs - 2}, Vector2{rx + fs, cy - 2}, 2, INK);
                hand(c.title, rx + fs + 8, cy, fs, Color{INK.r, INK.g, INK.b, 150}, 20); 
                float tw = mt(c.title, fs); DrawLineEx(Vector2{rx + fs + 6, cy + fs * 0.55f}, Vector2{rx + fs + 10 + tw, cy + fs * 0.5f}, 1.5f, Color{INK.r, INK.g, INK.b, 150});
                cy += lh;
            } else if (g.book.available(c)) {
                DrawRectangleLines((int)rx, (int)(cy + 2), fs - 3, fs - 3, INK);
                int j = 0; for (const string& ln : wrapText(c.hint, fs, mw - fs - 8)) { hand(ln, rx + fs + 8, cy + j * lh, fs, INK, 30 + j); j++; }
                cy += lh * j;
            } else hidden++;
        }
        if (hidden > 0) { hand("...and things I haven't thought to ask yet.", rx, cy + lh * 0.3f, fs, Color{INK.r, INK.g, INK.b, 160}, 55); cy += lh * 1.3f; }
        hand("Clues so far: " + std::to_string(g.book.found.size()) + " / " + std::to_string(g.book.ids.size()), rx, R.y + R.height - lh * 3.6f, fs, INK, 60);
        if (g.book.canConfront() && !g.solved) hand("Enough to lean on the man in the scarf.", rx, R.y + R.height - lh * 2.6f, fs, RED_INK, 61);
        else if (!g.solved) hand("Not enough to accuse anyone. Not yet.", rx, R.y + R.height - lh * 2.6f, fs, Color{INK.r, INK.g, INK.b, 170}, 62);
    }
    // ------------------------------------------------------------ NOTES (one diary entry per clue, in the order they were written)
    else if (g.journal.tab == 1 || g.journal.tab == 2) {
        vector<Block> bl;
        float tw = mw;
        if (g.journal.tab == 1) {
            hand("NOTES", L.x + mx, L.y + lh * 0.3f, fs + 4, RED_INK, 2); scribble(L.x + mx, L.x + mx + mt("NOTES", fs + 4), L.y + lh * 0.3f + fs + 6, 4, RED_INK);
            if (g.book.found.empty()) bl.push_back({3, [=](float x, float y, float) { hand("Nothing yet.", x, y, fs, INK, 70); hand("Knock on doors. Look closer.", x, y + lh, fs, INK, 71); }});
            for (const string& id : g.book.found) {
                const Clue& c = g.book.clue(id);
                vector<string> body = wrapText(c.text, fs, tw);
                string head = fmtTime(c.minute) + "  " + c.title, src = "- " + c.source;
                bool fresh = c.fresh; int seed = (int)std::hash<string>()(id) & 255;
                bl.push_back({(int)body.size() + 3, [=](float x, float y, float w) {
                    hand(head, x, y, fs, RED_INK, seed); scribble(x, x + std::min(w, mt(head, fs)), y + fs + 3, seed, RED_INK, 1.2f);
                    for (size_t i = 0; i < body.size(); i++) hand(body[i], x, y + lh * (float)(i + 1), fs, INK, seed + 3 + (int)i);
                    hand(src, x + 14, y + lh * (float)(body.size() + 1), fs - 1, Color{INK.r, INK.g, INK.b, 150}, seed + 9);
                    if (fresh) stamp("NEW", x + w - 34 * k, y + lh * 0.3f, fs, Color{168, 40, 36, 210}, -10);
                }});
                if (std::find(g.journal.seen.begin(), g.journal.seen.end(), id) == g.journal.seen.end()) g.journal.seen.push_back(id);
            }
        } else {
            hand("PEOPLE", L.x + mx, L.y + lh * 0.3f, fs + 4, RED_INK, 2); scribble(L.x + mx, L.x + mx + mt("PEOPLE", fs + 4), L.y + lh * 0.3f + fs + 6, 4, RED_INK);
            bool any = false;
            for (const auto& cp : g.cast) {
                const Character& n = *cp;
                if (!n.met) continue;
                any = true;
                float pbw = 72 * k + 12, textW = tw - pbw - 10;
                vector<string> role = wrapText(n.role, fs - 2, textW), imp = wrapText(n.script.impression, fs, textW);
                vector<vector<string>> nts; int rows = 1 + (int)role.size() + (int)imp.size();
                for (const string& s : n.notes) { nts.push_back(wrapText("- " + s, fs, textW)); rows += (int)nts.back().size(); }
                rows = std::max(4, rows) + 1;
                const Character* np = &n;
                bl.push_back({rows, [=](float x, float y, float) {
                    DrawRectangleLines((int)x - 2, (int)y + 2, (int)pbw, (int)(lh * 4), Color{60, 48, 36, 110});
                    sketch(*np, x + 4, y + 4, lh * 4 - 8);
                    float tx2 = x + pbw + 10; int r = 0;
                    hand(np->name, tx2, y, fs + 1, RED_INK, 100 + (int)np->name.size()); r++;
                    for (const string& s : role) { typed(s, tx2, y + lh * (float)r + 2, fs - 2, PAPER_DIM); r++; }
                    for (const string& s : imp) { hand(s, tx2, y + lh * (float)r, fs, INK, 120 + r); r++; }
                    for (const auto& nt : nts) for (const string& s : nt) { hand(s, tx2, y + lh * (float)r, fs, INK, 140 + r); r++; }
                }});
            }
            if (!any) bl.push_back({3, [=](float x, float y, float) { hand("Nobody yet.", x, y, fs, INK, 72); hand("Knock. Somebody always answers.", x, y + lh, fs, INK, 73); }});
        }
        g.journal.spread = std::max(0, g.journal.spread);
        int maxSp = flowBlocks(bl, 0, L, R, topOff, lh, mx, mw);          // measure first, then clamp, then draw the right spread
        g.journal.spread = std::min(g.journal.spread, maxSp - 1);
        flowBlocks(bl, g.journal.spread, L, R, topOff, lh, mx, mw);
        total = maxSp;
    }
    // ------------------------------------------------------------ BOARD (the clue graph, pinned up with red string)
    else {
        Rectangle B{L.x, L.y, bw - 2 * pad, L.height};
        tileFill(corkTex, B, 2.0f, WHITE);
        DrawRectangleLinesEx(Rectangle{B.x - 3, B.y - 3, B.width + 6, B.height + 6}, 6, Color{70, 44, 28, 255});
        hand("THE CASE BOARD", B.x + 18, B.y + 10, fs + 4, Color{250, 240, 210, 255}, 3);
        hand("red string = this led to that", B.x + 18, B.y + 10 + lh, fs - 1, Color{250, 240, 210, 190}, 4);
        int maxD = 0; for (const string& id : g.book.ids) maxD = std::max(maxD, g.book.clue(id).depth);
        vector<vector<string>> cols((size_t)maxD + 1);
        for (const string& id : g.book.ids) cols[(size_t)g.book.clue(id).depth].push_back(id);
        size_t maxN = 1; for (auto& c : cols) maxN = std::max(maxN, c.size());
        float top = B.y + lh * 2.6f, availH = B.height - lh * 2.6f - 12, cw = std::min(B.width / (float)cols.size() * 0.80f, 240 * k), ch = std::min(availH / (float)maxN * 0.84f, 120 * k);
        std::unordered_map<string, Rectangle> pos;
        for (size_t d = 0; d < cols.size(); d++)
            for (size_t j = 0; j < cols[d].size(); j++) {
                float cx = B.x + B.width * ((float)d + 0.5f) / (float)cols.size(), cyy = top + availH * ((float)j + 0.5f) / (float)cols[d].size();
                float jx = ((int)(hash32((uint32_t)d, (uint32_t)j, 3) & 15) - 8) * 0.8f, jy = ((int)(hash32((uint32_t)d, (uint32_t)j, 4) & 15) - 8) * 0.6f;
                pos[cols[d][j]] = Rectangle{cx - cw / 2 + jx, cyy - ch / 2 + jy, cw, ch};
            }
        for (const string& id : g.book.ids) {                                       // strings first, so the cards sit on top of them
            const Clue& c = g.book.clue(id);
            for (const string& r : c.req) {
                const Rectangle& a = pos[r]; const Rectangle& b = pos[id];
                Vector2 p0{a.x + a.width, a.y + a.height / 2}, p1{b.x, b.y + b.height / 2};
                bool lit = g.book.has(r) && c.found; Color sc = lit ? Color{186, 36, 32, 255} : Color{150, 40, 36, 190};
                Vector2 prev = p0;
                for (int i = 1; i <= 18; i++) {
                    float t = i / 18.0f, sag = std::sin(t * 3.14159f) * 14 * k;
                    Vector2 q{p0.x + (p1.x - p0.x) * t, p0.y + (p1.y - p0.y) * t + sag};
                    DrawLineEx(prev, q, lit ? 2.5f : 2.0f, sc); prev = q;
                }
            }
        }
        for (const string& id : g.book.ids) {
            const Clue& c = g.book.clue(id); const Rectangle& r = pos[id];
            DrawRectangle((int)r.x + 4, (int)r.y + 5, (int)r.width, (int)r.height, Color{0, 0, 0, 90});
            if (c.found) {
                DrawRectangleRec(r, Color{238, 226, 192, 255});
                typed(c.title, r.x + 10, r.y + 12, fs - 1, RED_INK);
                hand(fmtTime(c.minute), r.x + 10, r.y + 12 + lh * 0.9f, fs - 2, Color{INK.r, INK.g, INK.b, 190}, 5);
                int j = 0; for (const string& ln : wrapText(c.source, fs - 3, r.width - 18)) { if (j < 2) typed(ln, r.x + 10, r.y + 12 + lh * (1.8f + 0.75f * (float)j), fs - 3, PAPER_DIM); j++; }
                if (c.autoFound) stamp("DEDUCTION", r.x + r.width - 52 * k, r.y + r.height - 14, fs - 4, Color{40, 90, 130, 200}, -6);
            } else if (g.book.available(c)) {
                DrawRectangleRec(r, Color{210, 198, 164, 220});
                for (float x = r.x; x < r.x + r.width; x += 8) { DrawLine((int)x, (int)r.y, (int)std::min(x + 4, r.x + r.width), (int)r.y, Color{70, 50, 36, 255}); DrawLine((int)x, (int)(r.y + r.height), (int)std::min(x + 4, r.x + r.width), (int)(r.y + r.height), Color{70, 50, 36, 255}); }
                typed("?", r.x + 10, r.y + 8, fs + 6, RED_INK);
                int j = 0; for (const string& ln : wrapText(c.hint, fs - 3, r.width - 18)) { if (j < 3) hand(ln, r.x + 10, r.y + 8 + lh * (1.2f + 0.7f * (float)j), fs - 3, INK, 200 + j); j++; }
            } else {
                DrawRectangleRec(r, Color{150, 130, 100, 150});
                typed("???", r.x + 10, r.y + r.height / 2 - fs / 2, fs + 2, Color{70, 54, 40, 200});
            }
            DrawCircle((int)(r.x + r.width / 2), (int)r.y + 4, 5, Color{180, 30, 28, 255}); DrawCircle((int)(r.x + r.width / 2) - 1, (int)r.y + 3, 1.5f, Color{255, 170, 150, 255});
        }
    }

    // page numbers and key hints
    if (!board) {
        hand(std::to_string(g.journal.spread * 2 + 1), L.x + L.width / 2, L.y + L.height - lh * 0.9f, fs - 2, Color{INK.r, INK.g, INK.b, 140}, 8);
        hand(std::to_string(g.journal.spread * 2 + 2), R.x + R.width / 2, R.y + R.height - lh * 0.9f, fs - 2, Color{INK.r, INK.g, INK.b, 140}, 9);
    }
    string hint = "[Left/Right] tab    [1-4] tab" + string(total > 1 ? "    [Up/Down] turn page" : "") + "    [J / Esc] close the journal";
    typed(hint, bx + 8, by + bh + 14, 15, Color{150, 138, 110, 255});
}

// ---------------------------------------------------------------- dialogue box
void Ui::drawDialogue(const Game& g, int ww, int wh) {
    int fs = std::clamp(ww / 58, 16, 21); float lh = fs * 1.4f;
    float bw = std::min((float)ww - 24, 1000.0f), bx = (ww - bw) / 2;
    vector<string> lines = wrapText(g.talk.text, fs, bw - 40);
    float menuH = 0; vector<vector<string>> labs;
    for (const MenuItem& m : g.talk.menu) { labs.push_back(wrapText(std::to_string(labs.size() + 1) + ". " + m.label, fs - 1, bw - 70)); menuH += lh * (float)labs.back().size(); }
    float boxH = 22 + lh * (float)lines.size() + 16 + menuH + 28;
    float by = (float)wh - boxH - 12;
    DrawRectangle((int)bx, (int)by, (int)bw, (int)boxH, Color{8, 8, 11, 230});
    DrawRectangleLinesEx(Rectangle{bx, by, bw, boxH}, 2, Color{150, 112, 52, 255});
    // name plate
    string nm = g.talk.speaker; for (char& c : nm) c = (char)std::toupper((unsigned char)c);
    float npw = mt(nm, fs) + 28;
    DrawRectangle((int)bx + 16, (int)by - (int)(lh * 0.9f), (int)npw, (int)(lh * 1.3f), Color{150, 112, 52, 255});
    typed(nm, bx + 30, by - lh * 0.9f + (lh * 1.3f - fs) / 2, fs, Color{12, 10, 8, 255});
    // the answer, typed out
    int left = (int)g.talk.reveal; float y = by + 20;
    for (const string& ln : lines) {
        if (left <= 0) break;
        typed(ln.substr(0, (size_t)std::min<int>(left, (int)ln.size())), bx + 20, y, fs, Color{236, 228, 208, 255});
        left -= (int)ln.size() + 1; y += lh;
    }
    y = by + 20 + lh * (float)lines.size() + 10;
    DrawLine((int)bx + 16, (int)y, (int)(bx + bw - 16), (int)y, Color{150, 112, 52, 120});
    y += 8;
    menuRects.clear();
    for (size_t i = 0; i < g.talk.menu.size(); i++) {
        float h = lh * (float)labs[i].size();
        Rectangle r{bx + 12, y - 2, bw - 24, h + 2};
        menuRects.push_back(r);
        bool sel = (int)i == g.talk.sel;
        if (sel) DrawRectangleRec(r, Color{232, 176, 74, 38});
        Color c = g.talk.menu[i].dim ? Color{120, 118, 110, 255} : sel ? AMBER : Color{214, 206, 188, 255};
        if (sel) typed(">", bx + 18, y, fs - 1, AMBER);
        for (size_t j = 0; j < labs[i].size(); j++) typed(labs[i][j], bx + 38, y + lh * (float)j, fs - 1, c);
        y += h;
    }
    typed("[Up/Down] choose   [Enter/E] ask   [J] journal   [Esc] leave", bx + 20, by + boxH - 22, 13, Color{110, 106, 96, 255});
}

void Ui::drawEnding(const Game& g, int ww, int wh) {
    float a = std::min(1.0f, g.endT / 1.6f);
    DrawRectangle(0, 0, ww, wh, Color{4, 3, 3, (unsigned char)(a * 238)});
    if (g.endT < 1.2f) return;
    int fs = 17; float lh = 26;
    vector<string> ev; for (const string& id : g.book.found) ev.push_back(g.book.clue(id).title);
    float pw = std::min(640.0f, (float)ww - 40), ph = 250 + lh * (float)std::min<size_t>(ev.size(), 8), px = (ww - pw) / 2, py = (wh - ph) / 2;
    float t = g.endT - 1.2f, al = std::min(1.0f, t / 0.8f);
    DrawRectangle((int)px + 6, (int)py + 8, (int)pw, (int)ph, Color{0, 0, 0, (unsigned char)(120 * al)});
    DrawRectangle((int)px, (int)py, (int)pw, (int)ph, Color{232, 218, 180, (unsigned char)(255 * al)});
    Color tc{44, 40, 36, (unsigned char)(255 * al)}, ic{28, 34, 70, (unsigned char)(255 * al)};
    typed("PRECINCT 7 - CASE FILE 30-1112", px + 24, py + 20, fs, tc);
    DrawLine((int)px + 24, (int)py + 46, (int)(px + pw - 24), (int)py + 46, Color{60, 50, 40, (unsigned char)(160 * al)});
    typed("SUSPECT   Igor Sidorov, apt 203. In custody.", px + 24, py + 58, fs - 1, tc);
    typed("MOTIVE    Unpaid debt. Humiliation.", px + 24, py + 58 + lh, fs - 1, tc);
    typed("EVIDENCE  from the detective's journal:", px + 24, py + 58 + lh * 2, fs - 1, tc);
    for (size_t i = 0; i < ev.size() && i < 8; i++) hand("- " + ev[i], px + 52, py + 58 + lh * (3 + (float)i), fs, ic, 300 + (int)i);
    float yy = py + 58 + lh * (3 + (float)std::min<size_t>(ev.size(), 8)) + 8;
    hand("The watch said nothing the whole time.", px + 24, yy, fs, ic, 330);
    hand("For once I was grateful.", px + 24, yy + lh, fs, ic, 331);
    if (t > 0.6f) stamp("CASE CLOSED", px + pw - 130, py + ph - 70, 26, Color{40, 110, 60, (unsigned char)(220 * std::min(1.0f, (t - 0.6f) / 0.4f))}, -9);
    if (g.endT > 4.0f) { DrawText("[Enter] keep exploring     [Esc] quit", (int)px, (int)(py + ph + 16), 16, Color{170, 160, 130, 255}); }
}

void Ui::drawHud(const Game& g, Rectangle dst, int ww) {
    // letterbox bars while someone is at the door
    float bar = g.bars * dst.height * 0.075f;
    if (bar > 0.5f) { DrawRectangle((int)dst.x, 0, (int)dst.width, (int)bar, BLACK); DrawRectangle((int)dst.x, (int)(dst.height - bar), (int)dst.width, (int)bar + 1, BLACK); }
    // caption while knocking
    if (g.capT > 0 && !g.caption.empty()) {
        float a = std::min(1.0f, g.capT / 0.8f); int fs = (int)(dst.height / 14);
        int tw = MeasureText(g.caption.c_str(), fs);
        DrawText(g.caption.c_str(), (int)(ww / 2 - tw / 2) + 2, (int)(dst.height * 0.30f) + 2, fs, Color{0, 0, 0, (unsigned char)(200 * a)});
        DrawText(g.caption.c_str(), ww / 2 - tw / 2, (int)(dst.height * 0.30f), fs, Color{236, 226, 200, (unsigned char)(255 * a)});
    }
    if (g.mode == M_WALK || g.mode == M_TALK) {
        // the notebook in your pocket
        float x = dst.x + dst.width - 52, y = bar + 10;
        DrawRectangle((int)x + 3, (int)y + 3, 34, 44, Color{0, 0, 0, 120});
        DrawRectangle((int)x, (int)y, 34, 44, LEATHER); DrawRectangleLines((int)x, (int)y, 34, 44, Color{120, 88, 60, 255});
        DrawRectangle((int)x + 24, (int)y, 4, 40, Color{150, 34, 30, 255}); DrawRectangle((int)x + 6, (int)y + 14, 14, 2, Color{200, 170, 110, 200});
        typed("J", x + 11, y + 22, 18, Color{230, 200, 140, 255});
        bool fresh = false; for (const string& id : g.book.found) if (g.book.clue(id).fresh) fresh = true;
        if (fresh && std::fmod(g.time, 1.0f) < 0.6f) DrawCircle((int)x + 34, (int)y, 6, Color{220, 50, 40, 255});
    }
    if (g.toastT > 0) {
        float a = std::min(1.0f, g.toastT / 1.0f); int fs = 18;
        int tw = MeasureText(g.toast.c_str(), fs); float bx = ww / 2.0f - tw / 2.0f - 30, by = bar + 12;
        DrawRectangle((int)bx, (int)by, tw + 60, 34, Color{10, 9, 8, (unsigned char)(220 * a)});
        DrawRectangleLines((int)bx, (int)by, tw + 60, 34, Color{232, 176, 74, (unsigned char)(255 * a)});
        DrawLineEx(Vector2{bx + 10, by + 24}, Vector2{bx + 26, by + 8}, 4, Color{236, 200, 70, (unsigned char)(255 * a)}); DrawLineEx(Vector2{bx + 8, by + 27}, Vector2{bx + 11, by + 22}, 3, Color{210, 150, 140, (unsigned char)(255 * a)});
        DrawText(g.toast.c_str(), (int)bx + 38, (int)by + 8, fs, Color{236, 226, 200, (unsigned char)(255 * a)});
    }
}


// ---------------------------------------------------------------- frame
void Ui::init() {
    Image img = GenImageColor(RW, RH, BLACK);
    sceneTex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(sceneTex, TEXTURE_FILTER_POINT);
    paperTex = makeNoiseTex(256, 256, Color{228, 214, 176, 255}, 12, 11, 36, Color{212, 194, 154, 255});
    corkTex = makeNoiseTex(256, 256, Color{154, 112, 72, 255}, 34, 23, 220, Color{110, 76, 46, 255});
}

void Ui::shutdown() {
    UnloadTexture(sceneTex); UnloadTexture(paperTex); UnloadTexture(corkTex);
}

void Ui::pointerInput(Game& g, Input& in) {
    Vector2 mp = GetMousePosition();
    if (g.mode == M_TALK)
        for (size_t i = 0; i < menuRects.size(); i++)
            if (CheckCollisionPointRec(mp, menuRects[i])) { in.hover = (int)i; if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) in.pick = (int)i; }
    if (g.mode == M_JOURNAL && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        for (int i = 0; i < 4; i++) if (CheckCollisionPointRec(mp, tabRects[i])) in.tabPick = i;
}

void Ui::draw(Game& g, int ww, int wh) {
    if (g.mode != M_JOURNAL) UpdateTexture(sceneTex, g.renderer.pixels());
    ClearBackground(Color{7, 9, 12, 255});
    int logH = 118;
    float sc = std::min((float)ww / RW, (float)(wh - logH) / RH);
    Rectangle dst{(ww - RW * sc) / 2, 0, RW * sc, RH * sc};
    Rectangle shaken = dst;
    if (g.shake > 0.01f) {                                            // knocking / the lift rumbling
        uint32_t f = (uint32_t)(g.time * 60.0f);
        shaken.x += ((int)(hash32(f, 1, 2) & 15) - 7.5f) * g.shake * 0.9f;
        shaken.y += ((int)(hash32(f, 3, 4) & 15) - 7.5f) * g.shake * 0.9f;
    }
    DrawTexturePro(sceneTex, Rectangle{0, 0, (float)RW, (float)RH}, shaken, Vector2{0, 0}, 0, WHITE);
    drawHud(g, dst, ww);

    if (g.mode == M_WALK) {
        string prompt = g.prompt();
        if (!prompt.empty()) {
            int tw = MeasureText(prompt.c_str(), 20);
            DrawRectangle((int)(ww / 2 - tw / 2 - 10), (int)(dst.height - 58), tw + 20, 30, Color{0, 0, 0, 190});
            DrawText(prompt.c_str(), ww / 2 - tw / 2, (int)(dst.height - 52), 20, COL_ME);
        }
        const Level& L = g.building.current();
        const Cell& here = L.cellAt((int)std::floor(g.player.x), (int)std::floor(g.player.y));
        string where = here.stair ? "Stairs" : L.inCar(g.player.x, g.player.y) ? "Lift" : L.name;
        DrawText(where.c_str(), (int)dst.x + 12, 10, 18, Color{200, 210, 220, 200});
        DrawText(fmtTime(g.minutes()).c_str(), (int)(dst.x + dst.width) - 118, 22, 18, Color{200, 210, 220, 200});
    }
    if (g.mode == M_WALK || g.mode == M_KNOCK || g.mode == M_FADE || g.mode == M_RIDE || g.mode == M_PEEP) {
        int top = (int)dst.height;
        DrawRectangle(0, top, ww, wh - top, Color{10, 13, 16, 255});
        DrawText(g.mode == M_PEEP ? "PEEPHOLE   A/D or Left/Right look around   E / P / Esc step back"
                                  : "WASD move   Left/Right turn   TAB mouse look   E knock / talk / lift   P peephole   J journal   M mute", 10, top + 6, 14, COL_SYS);
        vector<std::pair<string, Color>> rows;
        int first = std::max(0, (int)g.log.size() - 3);
        for (int i = first; i < (int)g.log.size(); i++)
            for (const string& ln : wrapText(g.log[(size_t)i].text, 15, (float)ww - 24)) rows.push_back({ln, g.log[(size_t)i].col});
        int rf = std::max(0, (int)rows.size() - 5);
        for (int i = rf; i < (int)rows.size(); i++) DrawText(rows[(size_t)i].first.c_str(), 10, top + 28 + (i - rf) * 17, 15, rows[(size_t)i].second);
    }
    if (g.mode == M_TALK) drawDialogue(g, ww, wh);
    if (g.mode == M_END) drawEnding(g, ww, wh);
    if (g.fade > 0.002f) DrawRectangle(0, 0, ww, wh, Color{0, 0, 0, (unsigned char)(255.0f * std::min(1.0f, g.fade))});   // floor change / lift ride
    if (g.mode == M_JOURNAL) drawJournal(g, ww, wh);
}

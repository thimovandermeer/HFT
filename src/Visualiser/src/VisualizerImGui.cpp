#include "OrderBook.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <thread>
#include <utility>
#include <vector>
#include <string>
#include <optional>
#include <numeric>
#include <iostream>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include "implot.h"
#include "Visualizer.hpp"

#ifdef _WIN32
  #include <Windows.h>
#endif
#ifdef __APPLE__
  #include <pthread.h>
  #ifndef GL_SILENCE_DEPRECATION
  #define GL_SILENCE_DEPRECATION 1
  #endif
#endif

using namespace std::chrono;

static inline bool isnan_d(double x){ return std::isnan(x); }

struct Metrics {
    double spread = std::numeric_limits<double>::quiet_NaN();
    double mid    = std::numeric_limits<double>::quiet_NaN();
    double micro  = std::numeric_limits<double>::quiet_NaN();
    double imbn   = 0.0;
    int    N      = 5;
};

static double sum_top_n(const std::vector<std::pair<double,double>>& v, int n) {
    double s = 0.0; n = std::min<int>(n, (int)v.size());
    for (int i=0;i<n;++i) s += v[i].second; return s;
}

static Metrics compute_metrics(const OrderBookSnapshot& s, int nForImb) {
    Metrics m; m.N = nForImb;
    if (!isnan_d(s.bestBid) && !isnan_d(s.bestAsk)) {
        m.spread = s.bestAsk - s.bestBid;
        m.mid    = 0.5 * (s.bestAsk + s.bestBid);
    }
    if (!s.bidLevels.empty() && !s.askLevels.empty()) {
        const auto [Bb,Sb] = s.bidLevels.front();
        const auto [Ba,Sa] = s.askLevels.front();
        const double denom = Sb + Sa; if (denom > 0) m.micro = (Bb*Sa + Ba*Sb)/denom;
    }
    const double b = sum_top_n(s.bidLevels, nForImb);
    const double a = sum_top_n(s.askLevels, nForImb);
    const double tot = b + a; m.imbn = (tot > 0) ? (b - a)/tot : 0.0;
    return m;
}

static double compute_latency_us(const OrderBookSnapshot& s) {
    const auto now = steady_clock::now();
    return duration<double, std::micro>(now - s.mono_ts).count();
}

template<typename T>
struct Ring {
    std::vector<T> buf;
    size_t head=0;
    explicit Ring(size_t cap=2048):buf(cap) {}
    void push(const T& v){ buf[head]=v; head=(head+1)%buf.size(); }
};

struct SeriesF {
    Ring<double> y;
    Ring<double> t;
    explicit SeriesF(size_t cap=2048): y(cap), t(cap) {}
    void push(double ts, double v){ t.push(ts); y.push(v); }
};

struct HistF {
    std::vector<float> bins;
    float lo, hi;
    explicit HistF(int nbins=50, float lo_=0, float hi_=1):bins(nbins,0),lo(lo_),hi(hi_) {}
    void reset(){ std::fill(bins.begin(), bins.end(), 0); }
    void add(float v){
        if (v<lo||v>hi) return;
        int i = (int)((v-lo)/(hi-lo) * (bins.size()-1));
        if (i>=0 && i<(int)bins.size()) bins[i]+=1.f;
    }
};

struct DepthHeatmap {
    int T=200;
    int P=60;
    double px_min=0, px_max=0;
    std::vector<float> z;
    int col=0;

    DepthHeatmap(int TT=200,int PP=60):T(TT),P(PP),z(TT*PP,0.0f){}

    void reset_range(double pxmin, double pxmax) {
        px_min=pxmin; px_max=pxmax;
        std::fill(z.begin(), z.end(), 0.0f);
        col=0;
    }

    int price_to_row(double px) const {
        if (px_max<=px_min) return -1;
        double r = (px - px_min) / (px_max - px_min);
        int row = P-1 - (int)std::round(r*(P-1));
        if (row<0 || row>=P) return -1;
        return row;
    }

    void ingest(const OrderBookSnapshot& s) {
        if (px_max<=px_min || s.bidLevels.empty() || s.askLevels.empty()) return;
        const int N = 10;
        for (int side=0; side<2; ++side) {
            const auto& lvls = side==0 ? s.bidLevels : s.askLevels;
            int n = std::min((int)lvls.size(), N);
            for (int i=0;i<n;++i) {
                int row = price_to_row(lvls[i].first);
                if (row<0) continue;
                float v = (float)lvls[i].second;
                z[row*T + col] += v;
            }
        }
        col = (col+1)%T;
    }
};

static void draw_ladder(const OrderBookSnapshot& s, int depthToShow) {
    ImGui::SeparatorText("L2 Ladder");

    const int nb = std::min<int>(depthToShow, (int)s.bidLevels.size());
    const int na = std::min<int>(depthToShow, (int)s.askLevels.size());
    const int R  = std::max(nb, na);

    ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;
    if (ImGui::BeginTable("ladder", 6, flags)) {
        ImGui::TableSetupColumn("Bid Px");
        ImGui::TableSetupColumn("Bid Sz");
        ImGui::TableSetupColumn("Bid Cum");
        ImGui::TableSetupColumn("Ask Px");
        ImGui::TableSetupColumn("Ask Sz");
        ImGui::TableSetupColumn("Ask Cum");
        ImGui::TableHeadersRow();

        double cumB=0.0, cumA=0.0;
        for (int i=0;i<R;++i) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            if (i<nb) ImGui::TextColored(ImVec4(0.2f,1.0f,0.2f,1.0f), "%.2f", s.bidLevels[i].first);
            else      ImGui::TextDisabled("-");

            ImGui::TableSetColumnIndex(1);
            if (i<nb) { cumB += s.bidLevels[i].second; ImGui::Text("%.6f", s.bidLevels[i].second); }
            else      ImGui::TextDisabled("-");

            ImGui::TableSetColumnIndex(2);
            if (i<nb) ImGui::Text("%.6f", cumB);
            else      ImGui::TextDisabled("-");

            ImGui::TableSetColumnIndex(3);
            if (i<na) ImGui::TextColored(ImVec4(1.0f,0.3f,0.3f,1.0f), "%.2f", s.askLevels[i].first);
            else      ImGui::TextDisabled("-");

            ImGui::TableSetColumnIndex(4);
            if (i<na) { cumA += s.askLevels[i].second; ImGui::Text("%.6f", s.askLevels[i].second); }
            else      ImGui::TextDisabled("-");

            ImGui::TableSetColumnIndex(5);
            if (i<na) ImGui::Text("%.6f", cumA);
            else      ImGui::TextDisabled("-");
        }
        ImGui::EndTable();
    }
}

static void draw_metrics_and_charts(const OrderBookSnapshot& s, const Metrics& m,
                                    SeriesF& sBid, SeriesF& sAsk, SeriesF& sMicro,
                                    SeriesF& sSpread, SeriesF& sImb, SeriesF& sLat,
                                    HistF& hLat, double tsec)
{
    ImGui::SeparatorText("Metrics");
    ImGui::Text("Symbol: %s", s.symbol.c_str());
    ImGui::Text("BestBid: %s%.2f%s   BestAsk: %s%.2f%s",
        isnan_d(s.bestBid)?"(na)":"", isnan_d(s.bestBid)?0.0:s.bestBid, isnan_d(s.bestBid)?"":"",
        isnan_d(s.bestAsk)?"(na)":"", isnan_d(s.bestAsk)?0.0:s.bestAsk, isnan_d(s.bestAsk)?"":"");

    ImGui::Text("Spread: %.6f   Mid: %.6f   Micro: %.6f   Imbalance(top-%d): %.3f",
                m.spread, m.mid, m.micro, m.N, m.imbn);

    if (!s.bidLevels.empty())   sBid.push(tsec,  (float)s.bidLevels.front().first);
    if (!s.askLevels.empty())   sAsk.push(tsec,  (float)s.askLevels.front().first);
    if (!std::isnan(m.micro))   sMicro.push(tsec,(float)m.micro);
    if (!std::isnan(m.spread))  sSpread.push(tsec,(float)m.spread);

    const float lat = (float)compute_latency_us(s);
    sLat.push(tsec, lat);
    hLat.add(lat);

    if (ImPlot::BeginSubplots("Market State", 2, 2, ImVec2(-1, 420),
                              ImPlotSubplotFlags_LinkAllX|ImPlotSubplotFlags_NoTitle)) {

        if (ImPlot::BeginPlot("Top-of-Book")) {
            ImPlot::SetupAxes("t (s)", "price");
            ImPlot::PlotLine("Bid",  sBid.t.buf.data(),   sBid.y.buf.data(),   (int)sBid.y.buf.size(), 0, sBid.t.head);
            ImPlot::PlotLine("Ask",  sAsk.t.buf.data(),   sAsk.y.buf.data(),   (int)sAsk.y.buf.size(), 0, sAsk.t.head);
            ImPlot::PlotLine("Micro",sMicro.t.buf.data(), sMicro.y.buf.data(), (int)sMicro.y.buf.size(),0, sMicro.t.head);
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("Spread")) {
            ImPlot::SetupAxes("t (s)", "spread");
            ImPlot::PlotLine("spread", sSpread.t.buf.data(), sSpread.y.buf.data(), (int)sSpread.y.buf.size(), 0, sSpread.t.head);
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("Imbalance")) {
            ImPlot::SetupAxes("t (s)", "imb");
            ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0, ImGuiCond_Always);
            sImb.push(tsec, (float)m.imbn);
            ImPlot::PlotLine("imb", sImb.t.buf.data(), sImb.y.buf.data(), (int)sImb.y.buf.size(), 0, sImb.t.head);
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("Latency (us)")) {
            ImPlot::SetupAxes("t (s)", "us");
            ImPlot::PlotLine("lat", sLat.t.buf.data(), sLat.y.buf.data(), (int)sLat.y.buf.size(), 0, sLat.t.head);
            ImPlot::EndPlot();
        }

        ImPlot::EndSubplots();
    }

    if (ImPlot::BeginPlot("Latency Histogram")) {
        std::vector<double> xs(hLat.bins.size()), ys(hLat.bins.size());
        for (size_t i=0;i<hLat.bins.size();++i) {
            xs[i] = hLat.lo + (hLat.hi - hLat.lo) * (double)i / (double)std::max<size_t>(1,hLat.bins.size()-1);
            ys[i] = (double)hLat.bins[i];
        }
        ImPlot::PlotBars("latency", ys.data(), (int)ys.size(), (hLat.hi-hLat.lo)/hLat.bins.size(), xs.front());
        ImPlot::EndPlot();
    }
}

static void draw_heatmap(const DepthHeatmap& H) {
    if (H.z.empty() || H.px_max<=H.px_min) {
        ImGui::TextDisabled("Heatmap: waiting for range & data...");
        return;
    }
    ImPlot::ColormapScale("Depth", 0.0, 1.0, ImVec2(60, 300));
    ImPlot::PushColormap(ImPlotColormap_Viridis);
    if (ImPlot::BeginPlot("Depth Heatmap")) {
        ImPlot::SetupAxes("t-buckets", "price");
        std::vector<float> norm(H.z);
        for (int c=0;c<H.T;++c) {
            float m=0.f;
            for (int r=0;r<H.P;++r) m = std::max(m, H.z[r*H.T + c]);
            float inv = (m>0)? (1.f/m) : 0.f;
            for (int r=0;r<H.P;++r) norm[r*H.T + c] *= inv;
        }
        ImPlot::PlotHeatmap("depth", norm.data(), H.P, H.T, 0.0, 1.0, nullptr, ImPlotPoint(0, H.px_min), ImPlotPoint(H.T, H.px_max));
        ImPlot::EndPlot();
    }
    ImPlot::PopColormap();
}

static void glfw_error_cb(int error, const char* desc) {
    std::fprintf(stderr, "[GLFW] error %d: %s\n", error, desc ? desc : "");
    std::fflush(stderr);
}


VisualizerImGui::VisualizerImGui(OrderBookView& view, int depthToShow, int fps)
    : view_(view), depthToShow_(depthToShow), fps_(fps), imbLevels_(5) {}

void VisualizerImGui::setDepthToShow(int depth){ depthToShow_ = std::max(1, depth); }
void VisualizerImGui::setImbalanceLevels(int n){ imbLevels_   = std::max(1, n); }

void VisualizerImGui::run() {
    std::cout << "[VisualizerImGui] Starting\n";

#ifdef __APPLE__
    if (!pthread_main_np()) {
        std::fprintf(stderr, "[VisualizerImGui] ERROR: run() must be called on the main thread on macOS.\n");
        return;
    }
#endif

    glfwSetErrorCallback(glfw_error_cb);

#ifdef __APPLE__
    glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
    glfwInitHint(GLFW_COCOA_MENUBAR,         GLFW_FALSE);
#endif

    if (!glfwInit()) {
        std::fprintf(stderr, "[VisualizerImGui] glfwInit() failed\n");
        return;
    }

#ifdef __APPLE__
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    GLFWwindow* window = glfwCreateWindow(1400, 900, "HFT L2 Visualizer", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "[VisualizerImGui] Failed to create GLFW window\n");
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwShowWindow(window);
    glfwFocusWindow(window);
#ifdef __APPLE__
    glfwRequestWindowAttention(window);
#endif
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED))
        glfwRestoreWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();               // NEW
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        std::fprintf(stderr, "[VisualizerImGui] ImGui_ImplOpenGL3_Init failed (GLSL=%s)\n", glsl_version);
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return;
    }

    SeriesF sBid(1024), sAsk(1024), sMicro(1024), sSpread(1024), sImb(1024), sLat(1024);
    HistF   hLat(64, 0.f, 5000.f); // 0..5ms buckets
    auto t0 = steady_clock::now();

    DepthHeatmap heat(200, 80);
    bool heat_range_set = false;

    bool paused = false;

    const auto frameDur = milliseconds(1000 / std::max(1, fps_));
    OrderBookSnapshot snap;

    std::cout << "[VisualizerImGui] Running\n";
    while (!glfwWindowShouldClose(window)) {
        if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) paused = !paused;
        if (ImGui::IsKeyPressed(ImGuiKey_Equal, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd, false)) setDepthToShow(depthToShow_+1);
        if (ImGui::IsKeyPressed(ImGuiKey_Minus, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract, false)) setDepthToShow(std::max(1, depthToShow_-1));
        if (ImGui::IsKeyPressed(ImGuiKey_1, false)) setImbalanceLevels(1);
        if (ImGui::IsKeyPressed(ImGuiKey_2, false)) setImbalanceLevels(2);
        if (ImGui::IsKeyPressed(ImGuiKey_3, false)) setImbalanceLevels(3);
        if (ImGui::IsKeyPressed(ImGuiKey_4, false)) setImbalanceLevels(4);
        if (ImGui::IsKeyPressed(ImGuiKey_5, false)) setImbalanceLevels(5);

        if (!paused) {
            snap = view_.read();
        }

        if (!heat_range_set && !std::isnan(snap.bestBid) && !std::isnan(snap.bestAsk)) {
            double mid = 0.5*(snap.bestBid + snap.bestAsk);
            double span = std::max(1.0, 50.0 * std::max(1e-6, snap.bestAsk - snap.bestBid));
            heat.reset_range(mid - span, mid + span);
            heat_range_set = true;
        }
        if (!paused && heat_range_set) {
            heat.ingest(snap);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Order Book");
        ImGui::Text("Pause [Space]=%s | Depth [+/-]=%d | Imb N [1..5]=%d", paused?"ON":"OFF", depthToShow_, imbLevels_);

        Metrics m = compute_metrics(snap, imbLevels_);
        const double tsec = duration<double>(steady_clock::now() - t0).count();
        if (!paused) draw_metrics_and_charts(snap, m, sBid, sAsk, sMicro, sSpread, sImb, sLat, hLat, tsec);

        draw_ladder(snap, depthToShow_);
        ImGui::End();

        ImGui::Begin("Depth Heatmap");
        if (ImPlot::BeginPlot("Depth Intensity")) {
            ImPlot::EndPlot();
        }
        draw_heatmap(heat);
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.07f, 0.07f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();

        std::this_thread::sleep_for(frameDur);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

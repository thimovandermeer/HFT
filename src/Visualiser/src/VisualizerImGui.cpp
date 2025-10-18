
#include <iostream>
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

// ===== ImGui + GLFW backends =====
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include "Visualizer.hpp"

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
    double micro  = std::numeric_limits<double>::quiet_NaN(); // top-level microprice
    double imbN   = 0.0; // imbalance over top-N
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
    const double tot = b + a; m.imbN = (tot > 0) ? (b - a)/tot : 0.0;
    return m;
}

static double compute_latency_us(const OrderBookSnapshot& s) {
    const auto now = steady_clock::now();
    return duration<double, std::micro>(now - s.mono_ts).count();
}

struct RollingSeries {
    std::vector<float> y; size_t head = 0;
    explicit RollingSeries(size_t cap=1024) { y.assign(cap, 0.0f); }
    void push(float v){ y[head]=v; head=(head+1)%y.size(); }
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

static void draw_metrics_panel(const OrderBookSnapshot& s, const Metrics& m,
                               RollingSeries& spread, RollingSeries& micro,
                               RollingSeries& imb, RollingSeries& lat_us)
{
    ImGui::SeparatorText("Metrics");
    ImGui::Text("Symbol: %s", s.symbol.c_str());

    ImGui::Text("BestBid: %s%.2f%s   BestAsk: %s%.2f%s",
                isnan_d(s.bestBid)?"(na)":"", isnan_d(s.bestBid)?0.0:s.bestBid, isnan_d(s.bestBid)?"":"",
                isnan_d(s.bestAsk)?"(na)":"", isnan_d(s.bestAsk)?0.0:s.bestAsk, isnan_d(s.bestAsk)?"":"");

    ImGui::Text("Spread: %.6f   Mid: %.6f   Micro: %.6f   Imb(top-%d): %.3f",
                m.spread, m.mid, m.micro, m.N, m.imbN);

    const float lat = (float)compute_latency_us(s);

    if (!std::isnan((float)m.spread)) spread.push((float)m.spread);
    if (!std::isnan((float)m.micro))  micro.push((float)m.micro);
    imb.push((float)m.imbN);
    lat_us.push(lat);

    ImGui::PlotLines("Spread",      spread.y.data(), (int)spread.y.size(), (int)spread.head, nullptr, 0.0f, FLT_MAX, ImVec2(0, 70));
    ImGui::PlotLines("Microprice",  micro.y.data(),  (int)micro.y.size(),  (int)micro.head,  nullptr, 0.0f, FLT_MAX, ImVec2(0, 70));
    ImGui::PlotLines("Imbalance",   imb.y.data(),    (int)imb.y.size(),    (int)imb.head,    nullptr, -1.0f, 1.0f,   ImVec2(0, 70));
    ImGui::PlotLines("Latency (us)",lat_us.y.data(), (int)lat_us.y.size(), (int)lat_us.head, nullptr, 0.0f, FLT_MAX, ImVec2(0, 70));
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
    const char* glsl_version = "#version 150";      // GL 3.2 core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char* glsl_version = "#version 130";      // GL 3.0
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
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

    RollingSeries spread(512), micro(512), imb(512), lat_us(512);
    const auto frameDur = milliseconds(1000 / std::max(1, fps_));

    OrderBookSnapshot snap;

    std::cout << "[VisualizerImGui] Running\n";
    while (!glfwWindowShouldClose(window)) {
        snap = view_.read();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Order Book");
        Metrics m = compute_metrics(snap, imbLevels_);
        draw_metrics_panel(snap, m, spread, micro, imb, lat_us);
        draw_ladder(snap, depthToShow_);
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
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

#pragma once
#include <cstdint>

struct OrderBookSnapshot;
class OrderBookView;

class VisualizerImGui {
public:
	VisualizerImGui(OrderBookView& view, int depthToShow = 20, int fps = 60);
	void run();
	void setDepthToShow(int depth);
	void setImbalanceLevels(int n);

private:
	OrderBookView& view_;
	int depthToShow_;
	int fps_;
	int imbLevels_;
};
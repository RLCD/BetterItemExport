#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/wrappers/items/ItemsWrapper.h"
constexpr auto plugin_version = "1.0";

struct ProductData {
	int id;
	std::string productName;

	int slotId;
	std::string slot;

	int qualityId;
	std::string qualityName;


	bool isSpecialEdition;
	std::string specialEditionLabel;

	bool paintable;
	std::vector<int> compatibleProducts;

	std::string assetName;
	std::string thumbnailName;

	std::string DebugString();

};


struct ItemData {
	ProductData product;

	int paintId;
	std::string paintName;
};


class BetterItemExport: public BakkesMod::Plugin::BakkesModPlugin/*, public BakkesMod::Plugin::PluginWindow*/
{

	//std::shared_ptr<bool> enabled;

	//Boilerplate
	virtual void onLoad();
	virtual void onUnload();

	std::tuple<bool, SpecialEdition> IsSpecialEdition(ProductWrapper& prod);
	ProductData GetProductData(ProductWrapper& prod);

	void GetCompatibleProducts(ProductWrapper& prod, ProductData& data);

	void GetProductQuality(ProductWrapper& prod, ProductData& data);

	// Inherited via PluginWindow
	/*

	bool isWindowOpen_ = false;
	bool isMinimized_ = false;
	std::string menuTitle_ = "BetterItemExport";

	virtual void Render() override;
	virtual std::string GetMenuName() override;
	virtual std::string GetMenuTitle() override;
	virtual void SetImGuiContext(uintptr_t ctx) override;
	virtual bool ShouldBlockInput() override;
	virtual bool IsActiveOverlay() override;
	virtual void OnOpen() override;
	virtual void OnClose() override;
	
	*/
};


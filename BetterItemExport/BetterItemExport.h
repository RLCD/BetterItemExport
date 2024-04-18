#pragma once

#include <functional>
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/wrappers/items/ItemsWrapper.h"
#include "bakkesmod/wrappers/Engine/EnumWrapper.h" //included for update
constexpr auto plugin_version = "1.0";

enum class EQUIPSLOT {
	BODY = 0,
	DECAL = 1,
	WHEELS = 2,
	ROCKETBOOST = 3,
	ANTENNA = 4,
	TOPPER = 5,
	BUMPER = 6,
	PAINTFINISH = 7,
	BOT = 8,
	LOGO = 9,
	UNDERGLOW = 10,
	CRATES = 11,
	CUSTOMFINISH = 12,
	ENGINEAUDIO = 13,
	TRAIL = 14,
	GOALEXPLOSION = 15,
	PLAYERBANNER = 16,
	GARAGECOMPLEXROW = 17,
	GOALSTINGER = 18,
	PLAYERAVATAR = 19,
	AVATARBORDER = 20,
	PLAYERTITLE = 21,
	ESPORTSTEAM = 22,
	ARCHIVEDITEMS = 23,
	BLUEPRINTS = 24,
	SHOPITEM = 25,
	CURRENCY = 26
};

struct ProductData {
	int id;
	std::string productName;

	int slotId;
	std::string slot;

	int qualityId;
	std::string qualityName;

	bool paintable;
	std::vector<int> compatibleProducts;

	std::string assetName;
	std::string assetPath;
	std::string thumbnailName;

	std::string DebugString();

};

void to_json(json& j, const ProductData& p);


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

	std::tuple<bool, SpecialEdition> IsSpecialEdition(ProductWrapper& prod_to_check);
	ProductData GetProductData(ProductWrapper& prod);

	static void GetCompatibleProducts(ProductWrapper& prod, ProductData& data);

	static void GetProductQuality(ProductWrapper& prod, ProductData& data);

	void RLCDExport();

	std::vector<ProductData> GetProducts(std::function<bool(ProductData&)> filter = [](ProductData&){return true;});

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


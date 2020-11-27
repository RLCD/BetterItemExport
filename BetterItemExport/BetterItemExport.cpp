#include "pch.h"
#include "BetterItemExport.h"
#include <sstream>
#include <set>
#include <fstream>

BAKKESMOD_PLUGIN(BetterItemExport, "write a plugin description here", plugin_version, PLUGINTYPE_FREEPLAY)


void BetterItemExport::onLoad()
{
	//auto items = gameWrapper->GetItemsWrapper().GetAllProducts();
	//int i = 0;
	//for (auto item : items) {
	//	if (item.memory_address == 0) continue;
	//	auto prod = GetProductData(item);
	//	if (prod.paintable) {
	//		cvarManager->log("\n" + prod.DebugString());
	//	}
	//	i++;
	//	if (i > 50) break;
	//}
	//auto seDb = gameWrapper->GetItemsWrapper().GetSpecialEditionDB();
	//auto testProd = gameWrapper->GetItemsWrapper().GetProduct(4947);
	//auto prod = GetProductData(testProd);
	////seDb.

	//cvarManager->log("\n" + prod.DebugString());

	cvarManager->registerNotifier("rlcd_export", [this](auto args) {RLCDExport(); }, "", 0);

}

void BetterItemExport::onUnload()
{
}

std::tuple<bool, SpecialEdition> BetterItemExport::IsSpecialEdition(ProductWrapper& prod)
{

	static std::map<int, SpecialEdition> specialEditionSet;
	if (specialEditionSet.empty()) {
		auto allProducts = gameWrapper->GetItemsWrapper().GetAllProducts();
		for (auto prod : allProducts) {
			if (prod.memory_address == 0) continue;
			for (auto att : prod.GetAttributes()) {
				if (att.GetAttributeType() == "ProductAttribute_SpecialEditionSettings_TA") {
					auto se_attribute = ProductAttribute_SpecialEditionSettingsWrapper(att.memory_address);
					auto editions = se_attribute.GetEditions();
					for (auto edition : editions) {
						specialEditionSet.insert({ edition.productId, edition });
					}
				}
			}
		}
	}
	if (auto it = specialEditionSet.find(prod.GetID()); it != specialEditionSet.end()) {
		return {true, it->second};
	}
	else {
		return { false, SpecialEdition{} };
	}
	
}

ProductData BetterItemExport::GetProductData(ProductWrapper& prod)
{
	ProductData data;


	if (auto name = prod.GetLongLabel(); !name.IsNull()) {
		data.productName = name.ToString();
	}
	data.id = prod.GetID();

	if (auto slot = prod.GetSlot(); slot.memory_address != 0) {
		auto slotName = slot.GetLabel();
		data.slot = slotName.IsNull() ? "" : slot.GetLabel().ToString();
		data.slotId = slot.GetSlotIndex();
	}

	GetProductQuality(prod, data);

	data.assetName = prod.GetAssetPackageName();
	data.assetPath = prod.GetAssetPath().ToString();
	data.thumbnailName = prod.GetThumbnailAssetName();

	data.paintable = prod.IsPaintable();

	auto [isSe, edition] = IsSpecialEdition(prod);
	if (isSe) {
		data.productName += ": " + edition.label;
	}

	GetCompatibleProducts(prod, data);

	return data;
}

void BetterItemExport::GetCompatibleProducts(ProductWrapper& prod, ProductData& data)
{
	if (auto reqProd = prod.GetRequiredProduct(); reqProd.memory_address != 0) {
		data.compatibleProducts.push_back(reqProd.GetID());
	}
	for (auto att : prod.GetAttributes()) {
		if (att.GetAttributeType() == "ProductAttribute_BodyCompatibility_TA") {
			auto bodyCompt = ProductAttribute_BodyCompatibilityWrapper(att.memory_address);
			for (auto compatBody : bodyCompt.GetCompatibleBodies()) {
				data.compatibleProducts.push_back(compatBody.GetID());
			}
		}
	}
}

void BetterItemExport::GetProductQuality(ProductWrapper& prod, ProductData& data)
{
	auto quality = static_cast<PRODUCTQUALITY>(prod.GetQuality());
	data.qualityId = (int)quality;
	switch (quality)
	{
	case Common:
		data.qualityName = "Common";
		break;
	case Uncommon:
		data.qualityName = "Uncommon";
		break;
	case Rare:
		data.qualityName = "Rare";
		break;
	case VeryRare:
		data.qualityName = "Very Rare";
		break;
	case Import:
		data.qualityName = "Import";
		break;
	case Exotic:
		data.qualityName = "Exotic";
		break;
	case BlackMarket:
		data.qualityName = "Black Market";
		break;
	case Premium:
		data.qualityName = "Premium";
		break;
	case Limited:
		data.qualityName = "Limited";
		break;
	case 9: // TODO: replace with Legacy when it's available upstream
		data.qualityName = "Legacy";
		break;
	default:
		data.qualityName = "Unknown";
		break;
	}
}

void BetterItemExport::RLCDExport()
{
	std::set<EQUIPSLOT> slotsToExport{ 
		EQUIPSLOT::BODY, 
		EQUIPSLOT::ROCKETBOOST, 
		EQUIPSLOT::WHEELS, 
		EQUIPSLOT::ANTENNA, 
		EQUIPSLOT::TRAIL, 
		EQUIPSLOT::DECAL,
		EQUIPSLOT::TOPPER,
		EQUIPSLOT::PAINTFINISH };
	std::set<int> itemsToExclude{
		1412, // Mystery Universal Decal
		1470, // Black Market Preview
		3138, // Mystery Item
		3315, // Random Certified Item
		3316, // Random Painted Item
		5364, // Black Market Drop
		5365, // Exotic Drop
		5366, // Import Drop
		5367, // Rare Drop
		5368, // Uncommon Drop
		5369, // Very Rare Drop
	};
	auto itemsToExport = GetProducts([&](ProductData& prod) {
		if (prod.id != 0 &&
			slotsToExport.find((EQUIPSLOT)prod.slotId) != slotsToExport.end() &&
			itemsToExclude.find(prod.id) == itemsToExclude.end())
		{
			return true;
		}
		return false;
	});

	json j = itemsToExport;
	std::ofstream myfile;
	myfile.open("rlcd_export.json");
	if (myfile)
	{
		myfile << j.dump(4, ' ', true, json::error_handler_t::ignore);
	}
	myfile.close();
	std::ofstream csv("rlcd_export.csv");
	//print headers
	csv << "ID" << ","
		<< "Name" << ","
		<< "IsPaintable" << ","
		<< "Slot" << ","
		<< "AssetPath" << ","
		<< "Quality" << ","
		<< std::endl;

	// print item data
	for (auto item : itemsToExport) {
		csv 
			<< item.id << ","
			<< item.productName << ","
			<< (item.paintable ? "1": "0") << ","
			<< item.slot << ","
			<< item.assetPath << ","
			<< item.qualityName
			<< std::endl;
	}
	csv.close();
}

std::vector<ProductData> BetterItemExport::GetProducts(std::function<bool(ProductData&)> filter)
{
	auto items = gameWrapper->GetItemsWrapper().GetAllProducts();
	std::vector<ProductData> products;
	for (auto item : items) {
		if (item.memory_address == 0) continue;
		auto prod = GetProductData(item);
		if (filter(prod)) {
			products.push_back(prod);
		}
	}
	return products;
}

std::string ProductData::DebugString()
{
	std::stringstream ss;
	ss << "Name: " << productName << "(" << id << ")\n"
		<< "Slot: " << slot << "(" << slotId << ")\n"
		<< "Quality: " << qualityName << "(" << qualityId << ")\n"
		<< "Asset:" << assetName << "\n"
		<< "Thumbnail:" << thumbnailName << "\n"
		<< (paintable ? "Paintable\n" : "");

	if (!compatibleProducts.empty()) {
		ss << "Compatible bodies:\n";
		for (auto bodyId : compatibleProducts) {
			ss << "\t" << bodyId << "\n";
		}
	}
	

	return ss.str();
}

void to_json(json& j, const ProductData& p)
{
	j = json{ 
		{ "productName", p.productName }, 
		{ "id", p.id }, 
		{ "paintable", p.paintable }, 
		{ "qualityId", p.qualityId }, 
		{ "qualityName", p.qualityName }, 
		{ "slot", p.slot }, 
		{ "slotId", p.slotId }, 
		{ "compatibleProducts", p.compatibleProducts },
		{ "assetName", p.assetName },
		{ "thumbnailName", p.thumbnailName },
	};
}

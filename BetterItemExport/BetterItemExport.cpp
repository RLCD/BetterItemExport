#include "pch.h"
#include "BetterItemExport.h"
#include <sstream>
#include <set>


BAKKESMOD_PLUGIN(BetterItemExport, "write a plugin description here", plugin_version, PLUGINTYPE_FREEPLAY)


void BetterItemExport::onLoad()
{
	auto items = gameWrapper->GetItemsWrapper().GetAllProducts();
	int i = 0;
	for (auto item : items) {
		if (item.memory_address == 0) continue;
		auto prod = GetProductData(item);
		if (prod.paintable) {
			cvarManager->log("\n" + prod.DebugString());
		}
		i++;
		if (i > 50) break;
	}
	auto seDb = gameWrapper->GetItemsWrapper().GetSpecialEditionDB();
	auto testProd = gameWrapper->GetItemsWrapper().GetProduct(4947);
	auto prod = GetProductData(testProd);
	//seDb.

	cvarManager->log("\n" + prod.DebugString());

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
		//for (auto& [productId,edition] : specialEditionSet) {
		//	std::stringstream ss;
		//	//auto seName = seDb.GetSpecialEditionName(edition);
		//	ss << "ProductId: " << productId << " EditionId: " << edition.editionId << " Name: " << edition.label;
		//	cvarManager->log(ss.str());
		//}
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
	data.thumbnailName = prod.GetThumbnailAssetName();

	data.paintable = prod.IsPaintable();

	auto [isSe, edition] = IsSpecialEdition(prod);
	data.isSpecialEdition = isSe;
	data.specialEditionLabel = edition.label;

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
	case MAX:
	default:
		data.qualityName = "Unknown";
		break;
	}
}

std::string ProductData::DebugString()
{
	std::stringstream ss;
	ss << "Name: " << productName << "(" << id << ")\n"
		<< "Slot: " << slot << "(" << slotId << ")\n"
		<< "Quality: " << qualityName << "(" << qualityId << ")\n"
		<< "Asset:" << assetName << "\n"
		<< "Thumbnail:" << thumbnailName << "\n"
		<< (paintable ? "Paintable\n" : "")
		<< (isSpecialEdition ? "Special edition:" + specialEditionLabel + "\n" : "");

	if (!compatibleProducts.empty()) {
		ss << "Compatible bodies:\n";
		for (auto bodyId : compatibleProducts) {
			ss << "\t" << bodyId << "\n";
		}
	}
	

	return ss.str();
}

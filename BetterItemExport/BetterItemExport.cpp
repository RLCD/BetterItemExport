#include "pch.h"
#include "BetterItemExport.h"
#include <sstream>
#include <set>
#include <fstream>

BAKKESMOD_PLUGIN(BetterItemExport, "write a plugin description here", plugin_version, PLUGINTYPE_FREEPLAY)


void BetterItemExport::onLoad()
{
	cvarManager->registerNotifier("rlcd_export", [this](...) {RLCDExport(); }, "", 0);
}

void BetterItemExport::onUnload()
{
}

std::tuple<bool, SpecialEdition> BetterItemExport::IsSpecialEdition(ProductWrapper& prod_to_check)
{
	static std::map<int, SpecialEdition> special_edition_set;
	if (special_edition_set.empty()) {
		auto all_products = gameWrapper->GetItemsWrapper().GetAllProducts();
		for (auto prod : all_products) {
			if (prod.memory_address == 0) continue;
			for (auto att : prod.GetAttributes()) {
				if (att.GetAttributeType() == "ProductAttribute_SpecialEditionSettings_TA") {
					auto se_attribute = ProductAttribute_SpecialEditionSettingsWrapper(att.memory_address);
					auto editions = se_attribute.GetEditions();
					for (const auto& edition : editions) {
						special_edition_set.insert({ edition.productId, edition });
					}
				}
			}
		}
	}
	if (auto it = special_edition_set.find(prod_to_check.GetID()); it != special_edition_set.end()) {
		return {true, it->second};
	}
	return { false, SpecialEdition{} };
}

ProductData BetterItemExport::GetProductData(ProductWrapper& prod)
{
	ProductData data;


	if (auto name = prod.GetLongLabel(); !name.IsNull()) {
		data.productName = name.ToString();
	}
	data.id = prod.GetID();

	if (auto slot = prod.GetSlot(); slot.memory_address != 0) {
		auto slot_name = slot.GetLabel();
		data.slot = slot_name.IsNull() ? "" : slot.GetLabel().ToString();
		data.slotId = slot.GetSlotIndex();
	}

	GetProductQuality(prod, data);

	data.assetName = prod.GetAssetPackageName();
	data.assetPath = prod.GetAssetPath().ToString();
	data.thumbnailName = prod.GetThumbnailAssetName();

	data.paintable = prod.IsPaintable();

	auto [isSe, edition] = IsSpecialEdition(prod);
	if (isSe) {
		std::string seTitle = ": " + edition.label;
		if (seTitle.size() > data.productName.size() ||
			data.productName.compare(data.productName.size() - seTitle.size(), seTitle.size(), seTitle) != 0) {
			data.productName += seTitle;
		}
	}

	GetCompatibleProducts(prod, data);

	return data;
}

void BetterItemExport::GetCompatibleProducts(ProductWrapper& prod, ProductData& data)
{
	if (auto req_prod = prod.GetRequiredProduct(); req_prod.memory_address != 0) {
		data.compatibleProducts.push_back(req_prod.GetID());
	}
	for (auto att : prod.GetAttributes()) {
		if (att.GetAttributeType() == "ProductAttribute_BodyCompatibility_TA") {
			auto body_compatibility_attribute = ProductAttribute_BodyCompatibilityWrapper(att.memory_address);
			for (auto compatible_body : body_compatibility_attribute.GetCompatibleBodies()) {
				if (compatible_body.IsNull()) continue;
				data.compatibleProducts.push_back(compatible_body.GetID());
			}
		}
	}
}

void BetterItemExport::GetProductQuality(ProductWrapper& prod, ProductData& data)
{
	data.qualityId = static_cast<int>(prod.GetQuality());
	const auto quality = static_cast<>(data.qualityId);
	//const auto quality = static_cast<PRODUCTQUALITY>(data.qualityId); //line causing issues. basically instead of using PRODUCTQUALITY get the enum from EnumWrapper::GetProductQualities() and save it?
	//var = static_cast<enum>(int);
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
	//default:
	//	data.qualityName = "Unknown";
	//	break;
	}
}

void BetterItemExport::RLCDExport()
{
	std::set<EQUIPSLOT> slots_to_export{ 
		EQUIPSLOT::BODY, 
		EQUIPSLOT::ROCKETBOOST, 
		EQUIPSLOT::WHEELS, 
		EQUIPSLOT::ANTENNA, 
		EQUIPSLOT::TRAIL, 
		EQUIPSLOT::DECAL,
		EQUIPSLOT::TOPPER,
		EQUIPSLOT::PAINTFINISH
	};
	std::set<int> items_to_exclude{
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
	std::ofstream log{gameWrapper->GetBakkesModPath() / "rlcd_export.log"};
	auto items_to_export = GetProducts([&](ProductData& prod) {
		if (prod.id != 0 &&
			slots_to_export.find(static_cast<EQUIPSLOT>(prod.slotId)) != slots_to_export.end() &&
			items_to_exclude.find(prod.id) == items_to_exclude.end())
		{
			return true;
		}
		return false;
	});

	// Find duplicate items, removing the lower/replaced
	for (auto i = items_to_export.begin(); i != items_to_export.end();) {
		// If there are any duplicate items with a higher id, remove this one
		const auto j = std::find_if(i + 1, items_to_export.end(), [&](auto item) {
			if ((*i).assetPath == item.assetPath)
				return true;
			return false;
		});

		if (j == items_to_export.end()) {
			++i;
		} else {
			const auto a = *i;
			const auto b = *j;
			log << "info: " << a.id << " (" << a.productName << ") and "<< b.id
				<< " (" << b.productName << ") have the same asset path ("
				<< a.assetPath << "), dropping " << a.id << "\n";
			i = items_to_export.erase(i);
		}
	}

	json j = items_to_export;
	std::ofstream myfile;
	myfile.open(gameWrapper->GetBakkesModPath() / "rlcd_export.json");
	if (myfile)
	{
		myfile << j.dump(4, ' ', true, json::error_handler_t::ignore);
	}
	myfile.close();
	std::ofstream csv(gameWrapper->GetBakkesModPath() / "rlcd_export.csv");
	//print headers
	csv << "ID" << ","
		<< "Name" << ","
		<< "IsPaintable" << ","
		<< "Slot" << ","
		<< "AssetPath" << ","
		<< "Quality"
		<< std::endl;

	// print item data
	for (const auto& item : items_to_export) {
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

std::vector<ProductData> BetterItemExport::GetProducts(const std::function<bool(ProductData&)> filter)
{
	auto items = gameWrapper->GetItemsWrapper().GetAllProducts();
	std::vector<ProductData> products;
	for (auto item : items) {
		if (item.IsNull()) continue;
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
		for (auto body_id : compatibleProducts) {
			ss << "\t" << body_id << "\n";
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

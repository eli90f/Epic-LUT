#pragma once
#include "item_preview.h"
namespace omega {
bool readPreviewCache(const std::wstring& key,ItemPreviewModel& model);
void writePreviewCache(const std::wstring& key,const ItemPreviewModel& model);
}

function entry_trait(obj)
	local ret = ""
	if obj:is(FOCUSED) then
		ret = T("IDS_TRAIT_TEXT_EDIT_FOCUSED_TV")
	else
		ret = T("IDS_TRAIT_TEXT_EDIT_TV")
	end
	return obj:roleName() .. ", " .. ret
end

function menu_item_trait(obj)
	n = table.getn(obj:parent():children())
	return string.format(T("IDS_TRAIT_MENU_ITEM_TAB_INDEX"), obj:index() + 1, n)
end

function popup_menu_trait(obj)
	local tr = { T("IDS_TRAIT_CTX_POPUP") , T("IDS_TRAIT_SHOWING") , tostring(table.getn(obj:children())),
				 T("IDS_TRAIT_ITEMS") , T("IDS_TRAIT_POPUP_CLOSE_TV")}
	return table.concat(tr, " ")
end

function dialog_trait(obj)
	local n = table.getn(obj:children())
	local ret = T("IDS_TRAIT_POPUP")
	if n > 0 then
		ret = ret .. T("IDS_TRAIT_SHOWING") .. " " .. tostring(n) .. " " .. T("IDS_TRAIT_ITEMS")
	end
	return ret .. T("IDS_TRAIT_POPUP_CLOSE_TV")
end

function list_item_trait(obj)
	local ret = ""
	local p = obj:parent()
	if p and p:is(MULTISELECTABLE) then
		if obj:is(SELECTED) then
			ret = ret .. T("IDS_TRAIT_ITEM_SELECTED")
		end
		if p:selection() then
			ret = ret .. string.format(T("IDS_TRAIT_ITEM_SELECTED_COUNT"), p:selection():count())
		end
	else
		if obj:is(EXPANDABLE) then
			if obj:is(EXPANDED) then
				ret = ret .. T("IDS_TRAIT_GROUP_INDEX_EXPANDED_TV")
			else
				ret = ret .. T("IDS_TRAIT_GROUP_INDEX_COLLAPSED_TV")
			end
		end
	end
	return ret
end

function check_radio_trait(obj)
	if obj:is(CHECKED) then
		return T("IDS_TRAIT_CHECK_BOX_SELECTED")
	else
		return T("IDS_TRAIT_CHECK_BOX_NOT_SELECTED")
	end
end

function push_button_trait(obj)
	return T("IDS_TRAIT_PUSH_BUTTON")
end

function progress_bar_trait(obj)
	if obj:value() then
		cv = obj:value():current()
		if cv >= 0 then
			return string.format(T("IDS_TRAIT_PD_PROGRESSBAR_PERCENT"), cv * 100)
		else
			return T("IDS_TRAIT_PD_PROGRESSBAR")
		end
	end
	return ""
end

function toggle_button_trait(obj)
	if obj:is(CHECKED) then
		local state = T("IDS_TRAIT_TOGGLE_BUTTON_ON")
	else
		local state = T("IDS_TRAIT_TOGGLE_BUTTON_OFF")
	end
	return T("IDS_TRAIT_TOGGLE_BUTTON") .. ", " .. state
end

function empty_trait(obj)
	return ""
end

function default_trait(obj)
	return obj:roleName()
end

local trait_map = {
	[ENTRY] = entry_trait,
	[MENU_ITEM] = menu_item_trait,
	[POPUP_MENU] = popup_menu_trait,
	[DIALOG] = dialog_trait,
	[LIST_ITEM] = list_item_trait,
	[CHECK_BOX] = check_radio_trait,
	[RADIO_BUTTON] = check_radio_trait,
	[PUSH_BUTTON] = push_button_trait,
	[PROGRESS_BAR] = progress_bar_trait,
	[TOGGLE_BUTTON] = toggle_button_trait,
	[HEADING] = empty_trait,
	[FILLER] = empty_trait,
}

function trait(obj)
	local func = trait_map[obj:role()]
	if func ~= nil then
		return func(obj)
	else
		return default_trait(obj)
	end
end

function describeObject(obj)
	local related_trait = {}
	for k, target in ipairs(obj:inRelation(RELATION_DESCRIBED_BY)) do
		table.insert(related_trait, trait(target))
	end
	local ret = {obj:name(), trait(obj), obj:description(), table.concat(related_trait, ", ")}
	for i=#ret,1,-1 do
		if ret[i] == nil or ret[i] == "" then
			table.remove(ret, i)
		end
	end
	return table.concat(ret, ", ")
end

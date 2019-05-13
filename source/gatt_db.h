
PRIMARY_SERVICE_UUID128(service_charges, uuid_charges)

	CHARACTERISTIC_UUID128(chargesDrogue_state, uuid_chargesDrogue_state, (gGattCharPropRead_c))
		VALUE_UUID128_VARLEN(chargesDrogue_state_value, uuid_chargesDrogue_Enable, (gPermissionFlagReadable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)

	CHARACTERISTIC_UUID128(chargesDrogue_Enable, uuid_chargesDrogue_Enable, (gGattCharPropWriteWithoutRsp_c))
		VALUE_UUID128_VARLEN(chargesDrogue_Enable_value, uuid_chargesDrogue_Enable, (gPermissionFlagWritable_c), gAttMaxReadDataSize_d(gAttMaxMtu_c), 1, 0x00)



	CHARACTERISTIC_UUID128(chargesMain_state, uuid_chargesMain_state, (gGattCharPropRead_c))
		VALUE_UUID128_VARLEN(chargesMain_state_value, uuid_chargesMain_Enable, (gPermissionFlagReadable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)

	CHARACTERISTIC_UUID128(chargesMain_Enable, uuid_chargesMain_Enable, (gGattCharPropWriteWithoutRsp_c))
		VALUE_UUID128_VARLEN(chargesMain_Enable_value, uuid_chargesMain_Enable, (gPermissionFlagWritable_c), gAttMaxReadDataSize_d(gAttMaxMtu_c), 1, 0x00)



	CHARACTERISTIC_UUID128(Ven_state, uuid_Ven_state, (gGattCharPropRead_c))
		VALUE_UUID128_VARLEN(Ven_state_value, uuid_Ven_Enable, (gPermissionFlagReadable_c), gAttMaxWriteDataSize_d(gAttMaxMtu_c), 1, 0x00)

	CHARACTERISTIC_UUID128(Ven_Enable, uuid_Ven_Enable, (gGattCharPropWriteWithoutRsp_c))
		VALUE_UUID128_VARLEN(Ven_Enable_value, uuid_Ven_Enable, (gPermissionFlagWritable_c), gAttMaxReadDataSize_d(gAttMaxMtu_c), 1, 0x00)

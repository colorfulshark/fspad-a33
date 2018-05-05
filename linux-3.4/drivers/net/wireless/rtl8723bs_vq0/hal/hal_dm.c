/******************************************************************************
 *
 * Copyright(c) 2014 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/

#include <drv_types.h>
#include <hal_data.h>

void Init_ODM_ComInfo(_adapter *adapter)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(adapter);
	EEPROM_EFUSE_PRIV	*pEEPROM = GET_EEPROM_EFUSE_PRIV(adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);
	struct mlme_ext_priv	*pmlmeext = &adapter->mlmeextpriv;
	struct mlme_priv	*pmlmepriv = &adapter->mlmepriv;
	struct pwrctrl_priv *pwrctl = adapter_to_pwrctl(adapter);
	int i;

	_rtw_memset(pDM_Odm,0,sizeof(*pDM_Odm));

	pDM_Odm->Adapter = adapter;

	ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_PLATFORM, ODM_CE);

	if (adapter->interface_type == RTW_GSPI)
		ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_INTERFACE, ODM_ITRF_SDIO);
	else
		ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_INTERFACE, adapter->interface_type);

	ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_MP_TEST_CHIP, IS_NORMAL_CHIP(pHalData->VersionID));

	ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_PATCH_ID, pEEPROM->CustomerID);

	ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_BWIFI_TEST, adapter->registrypriv.wifi_spec);

	if (pHalData->rf_type == RF_1T1R) {
		ODM_CmnInfoUpdate(pDM_Odm, ODM_CMNINFO_RF_TYPE, ODM_1T1R);
	}
	else if (pHalData->rf_type == RF_2T2R){
		ODM_CmnInfoUpdate(pDM_Odm, ODM_CMNINFO_RF_TYPE, ODM_2T2R);
	}
	else if (pHalData->rf_type == RF_1T2R){
		ODM_CmnInfoUpdate(pDM_Odm, ODM_CMNINFO_RF_TYPE, ODM_1T2R);
	}

	/* Pointer reference */
	ODM_CmnInfoHook(pDM_Odm, ODM_CMNINFO_TX_UNI, &(dvobj->traffic_stat.tx_bytes));
	ODM_CmnInfoHook(pDM_Odm, ODM_CMNINFO_RX_UNI, &(dvobj->traffic_stat.rx_bytes));
	ODM_CmnInfoHook(pDM_Odm, ODM_CMNINFO_WM_MODE, &(pmlmeext->cur_wireless_mode));
	ODM_CmnInfoHook(pDM_Odm, ODM_CMNINFO_BAND, &(pHalData->CurrentBandType));
	ODM_CmnInfoHook(pDM_Odm, ODM_CMNINFO_FORCED_RATE, &(pHalData->ForcedDataRate));

	ODM_CmnInfoHook(pDM_Odm, ODM_CMNINFO_SEC_CHNL_OFFSET, &(pHalData->nCur40MhzPrimeSC));
	ODM_CmnInfoHook(pDM_Odm, ODM_CMNINFO_SEC_MODE, &(adapter->securitypriv.dot11PrivacyAlgrthm));
	ODM_CmnInfoHook(pDM_Odm, ODM_CMNINFO_BW, &(pHalData->CurrentChannelBW));
	ODM_CmnInfoHook(pDM_Odm, ODM_CMNINFO_CHNL, &( pHalData->CurrentChannel));
	ODM_CmnInfoHook(pDM_Odm, ODM_CMNINFO_NET_CLOSED, &(adapter->net_closed));
	ODM_CmnInfoHook(pDM_Odm, ODM_CMNINFO_FORCED_IGI_LB, &(pHalData->u1ForcedIgiLb));

	ODM_CmnInfoHook(pDM_Odm, ODM_CMNINFO_SCAN, &(pmlmepriv->bScanInProcess));
	ODM_CmnInfoHook(pDM_Odm, ODM_CMNINFO_POWER_SAVING, &(pwrctl->bpower_saving));

	for(i=0; i<ODM_ASSOCIATE_ENTRY_NUM; i++)
		ODM_CmnInfoPtrArrayHook(pDM_Odm, ODM_CMNINFO_STA_STATUS, i, NULL);

	/* TODO */
	//ODM_CmnInfoHook(pDM_Odm, ODM_CMNINFO_BT_OPERATION, _FALSE);
	//ODM_CmnInfoHook(pDM_Odm, ODM_CMNINFO_BT_DISABLE_EDCA, _FALSE);
}

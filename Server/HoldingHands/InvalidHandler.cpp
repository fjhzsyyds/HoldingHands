#include "stdafx.h"
#include "InvalidHandler.h"
#include "utils.h"

CInvalidHandler::CInvalidHandler(CManager*pManager):
CMsgHandler(pManager)
{
}


CInvalidHandler::~CInvalidHandler()
{
}

void CInvalidHandler::OnOpen(){
	dbg_log("CInvalidHandler::OnOpen()");
	Close();
}
void CInvalidHandler::OnClose(){
	//什么也不做.
}
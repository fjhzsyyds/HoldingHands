#include "stdafx.h"
#include "InvalidHandler.h"
#include "utils.h"

CInvalidHandler::CInvalidHandler(CClientContext *pClient) :
CMsgHandler(pClient)
{
}


CInvalidHandler::~CInvalidHandler()
{
}

void CInvalidHandler::OnOpen(){
	Close();
}
void CInvalidHandler::OnClose(){
	//什么也不做.
}
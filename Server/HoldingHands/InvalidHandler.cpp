#include "stdafx.h"
#include "InvalidHandler.h"


CInvalidHandler::CInvalidHandler(CManager*pManager):
CMsgHandler(pManager)
{
}


CInvalidHandler::~CInvalidHandler()
{
}

void CInvalidHandler::OnOpen(){
	//直接关闭...
	Close();
}
void CInvalidHandler::OnClose(){
	//什么也不做.
}
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
	//ֱ�ӹر�...
	Close();
}
void CInvalidHandler::OnClose(){
	//ʲôҲ����.
}
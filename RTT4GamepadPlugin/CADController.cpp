#include "StdAfx.h"
#include "CADController.h"


CADController::CADController(void)
{
}


CADController::~CADController(void)
{
}

void CADController::Execute(HWND hWnd, LPCSTR szCommand, double deltaX, double deltaY) {}

void CADController::TumbleExecute(int deltaX, int deltaY) {}
void CADController::TrackExecute(int deltaX, int deltaY) {}
void CADController::DollyExecute(int deltaX, int deltaY) {}
void CADController::OriginalCommandExecute(LPCSTR command) {}
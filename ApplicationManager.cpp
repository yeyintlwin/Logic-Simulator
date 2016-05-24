#include "ApplicationManager.h"
#include "Actions\AddGate.h"
#include "Actions\AddConnection.h"
#include "Actions\Edit.h"
#include "Actions\Delete.h"
#include "Actions\Copy.h"
#include "Actions\Cut.h"
#include "Actions\Paste.h"
#include "Actions\UndoAction.h"
#include "Actions\RedoAction.h"
#include "Actions\Simulate.h"
#include "Actions\TruthTable.h"
#include "Actions\SaveAction.h"
#include "Actions\LoadAction.h"
#include "Actions\Select.h"
#include "Actions\Move.h"
#include "Actions\Hover.h"
using namespace std;

/* Constructor */
ApplicationManager::ApplicationManager() {
	mCompCount = 0;
	mCopiedComp = NULL;
	mCompList = new Component*[MAX_COMPONENTS];
	for (int i = 0; i < MAX_COMPONENTS; i++) mCompList[i] = NULL;

	// Creates the Input/Output objects and initialize the GUI
	pOut = new Output();
	pIn = pOut->CreateInput();
}

/* Returns the number of components */
int ApplicationManager::GetComponentsCount() const {
	return mCompCount;
}

/* Returns the list of components */
Component** ApplicationManager::GetComponentList() const {
	return mCompList;
}

/* Sets the last copied/cut component */
void ApplicationManager::SetCopiedComp(Component* pComp) {
	mCopiedComp = pComp;
}

/* Returns the last copied/cut component */
Component* ApplicationManager::GetCopiedComp() const {
	return mCopiedComp;
}

/* Returns a pointer to Input object */
Input* ApplicationManager::GetInput() {
	return pIn;
}

/* Returns a pointer to Output object */
Output* ApplicationManager::GetOutput() {
	return pOut;
}

/* Reads the required action from the user and returns the corresponding action type */
ActionType ApplicationManager::GetUserAction() {
	return pIn->GetUserAction(pOut);
}

/* Creates an action and executes it */
void ApplicationManager::ExecuteAction(ActionType &actType) {
	Action* pAct = NULL;

	switch (actType) {
		case ADD_GATE_AND:
		case ADD_GATE_OR:
		case ADD_GATE_NOT:
		case ADD_GATE_NAND:
		case ADD_GATE_NOR:
		case ADD_GATE_XOR:
		case ADD_GATE_XNOR:
		case ADD_GATE_AND3:
		case ADD_GATE_NOR3:
		case ADD_GATE_XOR3:
		case ADD_GATE_BUFFER:
		case ADD_SWITCH:
		case ADD_LED:
			pAct = new AddGate(this, actType);
			break;
		case ADD_CONNECTION:
			pAct = new AddConnection(this);
			break;
		case EDIT:
			pAct = new Edit(this);
			break;
		case DEL:
			pAct = new Delete(this);
			break;
		case COPY:
			pAct = new Copy(this);
			break;
		case CUT:
			pAct = new Cut(this);
			break;
		case PASTE:
			pAct = new Paste(this);
			break;
		case UNDO:
			pAct = new UndoAction(this);
			break;
		case REDO:
			pAct = new RedoAction(this);
			break;
		case SIMULATION_MODE:
			pOut->PrintMsg("SIMULATION MODE");
			UI.AppMode = Mode::SIMULATION;
			pOut->CreateToolBar();
			pAct = new Simulate(this);
			break;
		case DESIGN_MODE:
			pOut->PrintMsg("DESIGN MODE");
			UI.AppMode = Mode::DESIGN;
			pOut->CreateToolBar();
			pOut->CreateGateBar();
			//TODO:
			break;
		case CREATE_TRUTH_TABLE:
			pAct = new TruthTable(this);
			break;
		case SAVE:
			pAct = new SaveAction(this);
			break;
		case LOAD:
			pAct = new LoadAction(this);
			break;
		case SELECT:
			pAct = new Select(this);
			break;
		case MOVE:
			pAct = new Move(this);
			break;
		case HOVER:
			pAct = new Hover(this);
			break;
		case TOOL_BAR:
			//pOut->PrintMsg("TOOL BAR");
			break;
		case GATE_BAR:
			//pOut->PrintMsg("GATE BAR");
			break;
		case STATUS_BAR:
			//pOut->PrintMsg("STATUS BAR");
			break;
		case EXIT:
			Dialog *d = new Dialog("Would you like to save before exit?");
			if (d->GetUserClick() == YES)
				pAct = new SaveAction(this);
			else if (d->GetUserClick() == NO);
			else actType = DESIGN_MODE;
			delete d;
			break;
	}

	if (pAct) {
		if (pAct->Execute()) {
			mUndoStack.push(pAct);
			while (!mRedoStack.empty()) delete mRedoStack.top(), mRedoStack.pop();	// Clear the RedoStack
		}
		else {
			delete pAct;
		}
	}
}

/* Redraws all the drawing window */
void ApplicationManager::UpdateInterface() {
	for (int i = 0; i < mCompCount; i++) mCompList[i]->Draw(pOut);
	pOut->UpdateScreen();
}

/* Adds a new component to the list of components */
void ApplicationManager::AddComponent(Component* pComp) {
	if (mCompCount < MAX_COMPONENTS) {
		mCompList[mCompCount++] = pComp;
	}
}

/* Deselects all the components */
void ApplicationManager::DeselectComponents() {
	for (int i = 0; i < mCompCount; i++) {
		mCompList[i]->SetSelected(false);
	}
}

/* Counts and returns the number of selected components */
int ApplicationManager::CountSelectedComponents() {
	int n = 0;

	for (int i = 0; i < mCompCount; i++) {
		if (!mCompList[i]->IsDeleted() && mCompList[i]->IsSelected()) {
			n++;
		}
	}

	return n;
}

/* Undoes the last action */
void ApplicationManager::Undo() {
	if (mUndoStack.empty()) {
		return;
	}

	Action* lastAction = mUndoStack.top();
	lastAction->Undo();

	mUndoStack.pop();
	mRedoStack.push(lastAction);
}

/* Redoes the last action */
void ApplicationManager::Redo() {
	if (mRedoStack.empty()) {
		return;
	}

	Action* lastAction = mRedoStack.top();
	lastAction->Redo();

	mRedoStack.pop();
	mUndoStack.push(lastAction);
}

/* Saves the current circuit */
void ApplicationManager::Save(ofstream& file) {
	for (int i = 0; i < mCompCount; i++) {
		if (!mCompList[i]->IsDeleted()) {
			mCompList[i]->Save(file);
		}
	}
}

/* Loads the circuit from the file */
void ApplicationManager::Load(ifstream& file) {
	Data compData;
	string compType;
	Action* pAct;

	while (file >> compType, compType != "-1") {
		if (compType == "CONNECTION") {
			file >> compData.Label;
			file >> compData.GfxInfo.x1 >> compData.GfxInfo.y1 >> compData.GfxInfo.x2 >> compData.GfxInfo.y2;
			pAct = new AddConnection(this, &compData);
		}
		else {
			file >> compData.Label;
			file >> compData.GfxInfo.x1 >> compData.GfxInfo.y1;

			if (compType == "AND")
				pAct = new AddGate(this, ADD_GATE_AND, &compData);
			else if (compType == "OR")
				pAct = new AddGate(this, ADD_GATE_OR, &compData);
			else if (compType == "NOT")
				pAct = new AddGate(this, ADD_GATE_NOT, &compData);
			else if (compType == "NAND")
				pAct = new AddGate(this, ADD_GATE_NAND, &compData);
			else if (compType == "NOR")
				pAct = new AddGate(this, ADD_GATE_NOR, &compData);
			else if (compType == "XOR")
				pAct = new AddGate(this, ADD_GATE_XOR, &compData);
			else if (compType == "XNOR")
				pAct = new AddGate(this, ADD_GATE_XNOR, &compData);
			else if (compType == "AND3")
				pAct = new AddGate(this, ADD_GATE_AND3, &compData);
			else if (compType == "NOR3")
				pAct = new AddGate(this, ADD_GATE_NOR3, &compData);
			else if (compType == "XOR3")
				pAct = new AddGate(this, ADD_GATE_XOR3, &compData);
			else if (compType == "BUFFER")
				pAct = new AddGate(this, ADD_GATE_BUFFER, &compData);
			else if (compType == "SWITCH")
				pAct = new AddGate(this, ADD_SWITCH, &compData);
			else if (compType == "LED")
				pAct = new AddGate(this, ADD_LED, &compData);
		}

		pAct->Execute();
		delete pAct;
	}
}

/* Releases all the memory used by the components */
void ApplicationManager::ReleaseMemory() {
	for (int i = 0; i < mCompCount; i++) {
		mCompList[i]->Delete(pOut);
		delete mCompList[i];
	}

	mCompCount = 0;
	mCopiedComp = NULL;
	for (int i = 0; i < MAX_COMPONENTS; i++) mCompList[i] = NULL;

	while (!mUndoStack.empty()) delete mUndoStack.top(), mUndoStack.pop();
	while (!mRedoStack.empty()) delete mRedoStack.top(), mRedoStack.pop();
}

/* Destructor */
ApplicationManager::~ApplicationManager() {
	while (!mUndoStack.empty()) delete mUndoStack.top(), mUndoStack.pop();
	while (!mRedoStack.empty()) delete mRedoStack.top(), mRedoStack.pop();

	for (int i = 0; i < mCompCount; i++) delete mCompList[i];
	delete[] mCompList;
	delete pIn;
	delete pOut;
}
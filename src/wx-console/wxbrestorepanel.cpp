/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2004-2008 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 *
 *   wxbPanel for restoring files
 *
 *    Nicolas Boichat, April-July 2004
 *
 *    Version $Id$
 */

/* Note concerning "done" output (modifiable marked with +)
Run Restore job
+JobName:    RestoreFiles
Bootstrap:  /var/lib/bacula/restore.bsr
+Where:      /tmp/bacula-restores
+Replace:    always
+FileSet:    Full Set
+Client:     tom-fd
+Storage:    File
+When:       2004-04-18 01:18:56
+Priority:   10
OK to run? (yes/mod/no):mod
Parameters to modify:
     1: Level (not appropriate)
     2: Storage (automatic ?)
     3: Job (no)
     4: FileSet (yes)
     5: Client (yes : "The defined Client resources are:\n\t1: velours-fd\n\t2: tom-fd\nSelect Client (File daemon) resource (1-2):")
     6: When (yes : "Please enter desired start time as YYYY-MM-DD HH:MM:SS (return for now):")
     7: Priority (yes : "Enter new Priority: (positive integer)")
     8: Bootstrap (?)
     9: Where (yes : "Please enter path prefix for restore (/ for none):")
    10: Replace (yes : "Replace:\n 1: always\n 2: ifnewer\n 3: ifolder\n 4: never\n 
          Select replace option (1-4):")
    11: JobId (no)
Select parameter to modify (1-11):
       */

/*  Windows debug builds set _DEBUG which is used by wxWidgets to select their
 *  debug memory allocator.  Unfortunately it conflicts with Bacula's SmartAlloc.
 * So we turn _DEBUG off since we aren't interested in things it enables.
 */

#undef _DEBUG

#include "bacula.h"

#include "wxbrestorepanel.h"
#include "wxbmainframe.h"
#include "csprint.h"
#include <wx/choice.h>
#include <wx/datetime.h>
#include <wx/timer.h>
#include "unmarked.xpm"
#include "marked.xpm"
#include "partmarked.xpm"
#include <wx/imaglist.h>
#include <wx/listimpl.cpp>

/* A macro named Yield is defined under MinGW */
#undef Yield

WX_DEFINE_LIST(wxbEventList);

/*
 *  Class which is stored in the tree and in the list to keep informations
 *  about the element.
 */
class wxbTreeItemData : public wxTreeItemData {
   public:
      wxbTreeItemData(wxString path, wxString name, int marked, long listid = -1);
      ~wxbTreeItemData();
      wxString GetPath();
      wxString GetName();
      
      int GetMarked();
      void SetMarked(int marked);
      
      long GetListId();
   private:
      wxString* path; /* Full path */
      wxString* name; /* File name */
      int marked; /* 0 - Not Marked, 1 - Marked, 2 - Some file under is marked */
      long listid; /* list ID : >-1 if this data is in the list (and/or on the tree) */
};

wxbTreeItemData::wxbTreeItemData(wxString path, wxString name, int marked, long listid): wxTreeItemData() {
   this->path = new wxString(path);
   this->name = new wxString(name);
   this->marked = marked;
   this->listid = listid;
}

wxbTreeItemData::~wxbTreeItemData() {
   delete path;
   delete name;
}

int wxbTreeItemData::GetMarked() {
   return marked;
}

void wxbTreeItemData::SetMarked(int marked) {
   this->marked = marked;
}

long wxbTreeItemData::GetListId() {
   return listid;
}

wxString wxbTreeItemData::GetPath() {
   return *path;
}

wxString wxbTreeItemData::GetName() {
   return *name;
}

// ----------------------------------------------------------------------------
// event tables and other macros for wxWindows
// ----------------------------------------------------------------------------

enum
{
   RestoreStart = 1,
   RestoreCancel = 2,
   TreeCtrl = 3,
   ListCtrl = 4,
   ConfigOk = 5,
   ConfigApply = 6,
   ConfigCancel = 7,
   ConfigWhere = 8,
   ConfigReplace = 9,
   ConfigWhen = 10,
   ConfigPriority = 11,
   ConfigClient = 12,
   ConfigFileset = 13,
   ConfigStorage = 14,
   ConfigJobName = 15,
   ConfigPool = 16,
   TreeAdd = 17,
   TreeRemove = 18,
   TreeRefresh = 19,
   ListAdd = 20,
   ListRemove = 21,
   ListRefresh = 22
};

BEGIN_EVENT_TABLE(wxbRestorePanel, wxPanel)
   EVT_BUTTON(RestoreStart, wxbRestorePanel::OnStart)
   EVT_BUTTON(RestoreCancel, wxbRestorePanel::OnCancel)
   
   EVT_TREE_SEL_CHANGING(TreeCtrl, wxbRestorePanel::OnTreeChanging)
   EVT_TREE_SEL_CHANGED(TreeCtrl, wxbRestorePanel::OnTreeChanged)
   EVT_TREE_ITEM_EXPANDING(TreeCtrl, wxbRestorePanel::OnTreeExpanding)
   EVT_TREE_MARKED_EVENT(TreeCtrl, wxbRestorePanel::OnTreeMarked)
   EVT_BUTTON(TreeAdd, wxbRestorePanel::OnTreeAdd)
   EVT_BUTTON(TreeRemove, wxbRestorePanel::OnTreeRemove)
   EVT_BUTTON(TreeRefresh, wxbRestorePanel::OnTreeRefresh)

   EVT_LIST_ITEM_ACTIVATED(ListCtrl, wxbRestorePanel::OnListActivated)
   EVT_LIST_MARKED_EVENT(ListCtrl, wxbRestorePanel::OnListMarked)
   EVT_LIST_ITEM_SELECTED(ListCtrl, wxbRestorePanel::OnListChanged)
   EVT_LIST_ITEM_DESELECTED(ListCtrl, wxbRestorePanel::OnListChanged)
   EVT_BUTTON(ListAdd, wxbRestorePanel::OnListAdd)
   EVT_BUTTON(ListRemove, wxbRestorePanel::OnListRemove)
   EVT_BUTTON(ListRefresh, wxbRestorePanel::OnListRefresh)
  
   EVT_TEXT(ConfigWhere, wxbRestorePanel::OnConfigUpdated)
   EVT_TEXT(ConfigWhen, wxbRestorePanel::OnConfigUpdated)
   EVT_TEXT(ConfigPriority, wxbRestorePanel::OnConfigUpdated)
   EVT_CHOICE(ConfigWhen, wxbRestorePanel::OnConfigUpdated)
   EVT_CHOICE(ConfigReplace, wxbRestorePanel::OnConfigUpdated)
   EVT_CHOICE(ConfigClient, wxbRestorePanel::OnConfigUpdated)
   EVT_CHOICE(ConfigFileset, wxbRestorePanel::OnConfigUpdated)
   EVT_CHOICE(ConfigStorage, wxbRestorePanel::OnConfigUpdated)
   EVT_CHOICE(ConfigJobName, wxbRestorePanel::OnConfigUpdated)
   EVT_CHOICE(ConfigPool, wxbRestorePanel::OnConfigUpdated)
   
   EVT_BUTTON(ConfigOk, wxbRestorePanel::OnConfigOk)
   EVT_BUTTON(ConfigApply, wxbRestorePanel::OnConfigApply)
   EVT_BUTTON(ConfigCancel, wxbRestorePanel::OnConfigCancel)
END_EVENT_TABLE()

/*
 *  wxbRestorePanel constructor
 */
wxbRestorePanel::wxbRestorePanel(wxWindow* parent): wxbPanel(parent) 
{
   //pendingEvents = new wxbEventList(); //EVTQUEUE
   //processing = false; //EVTQUEUE
   SetWorking(false);
   
   imagelist = new wxImageList(16, 16, TRUE, 3);
   imagelist->Add(wxIcon(unmarked_xpm));
   imagelist->Add(wxIcon(marked_xpm));
   imagelist->Add(wxIcon(partmarked_xpm));

   wxFlexGridSizer* mainSizer = new wxFlexGridSizer(3, 1, 10, 10);
   mainSizer->AddGrowableCol(0);
   mainSizer->AddGrowableRow(1);

   wxFlexGridSizer *firstSizer = new wxFlexGridSizer(1, 2, 10, 10);

   firstSizer->AddGrowableCol(0);
   firstSizer->AddGrowableRow(0);

   start = new wxButton(this, RestoreStart, _("Enter restore mode"), wxDefaultPosition, wxSize(150, 30));
   firstSizer->Add(start, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 10);

   cancel = new wxButton(this, RestoreCancel, _("Cancel restore"), wxDefaultPosition, wxSize(150, 30));
   firstSizer->Add(cancel, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_RIGHT, 10);

   wxString elist[1];

/*   clientChoice = new wxChoice(this, ClientChoice, wxDefaultPosition, wxSize(150, 30), 0, elist);
   firstSizer->Add(clientChoice, 1, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 10);

   jobChoice = new wxChoice(this, -1, wxDefaultPosition, wxSize(150, 30), 0, elist);
   firstSizer->Add(jobChoice, 1, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 10);*/

   mainSizer->Add(firstSizer, 1, wxEXPAND, 10);

   treelistPanel = new wxSplitterWindow(this);

   wxPanel* treePanel = new wxPanel(treelistPanel);
   wxFlexGridSizer *treeSizer = new wxFlexGridSizer(2, 1, 0, 0);
   treeSizer->AddGrowableCol(0);
   treeSizer->AddGrowableRow(0);

   tree = new wxbTreeCtrl(treePanel, this, TreeCtrl, wxDefaultPosition, wxSize(200,50));  
   tree->SetImageList(imagelist);
   
   treeSizer->Add(tree, 1, wxEXPAND, 0);
   
   wxBoxSizer *treeCtrlSizer = new wxBoxSizer(wxHORIZONTAL);
   treeadd = new wxButton(treePanel, TreeAdd, _("Add"), wxDefaultPosition, wxSize(60, 25));
   treeCtrlSizer->Add(treeadd, 0, wxLEFT | wxRIGHT, 3);
   treeremove = new wxButton(treePanel, TreeRemove, _("Remove"), wxDefaultPosition, wxSize(60, 25));
   treeCtrlSizer->Add(treeremove, 0, wxLEFT | wxRIGHT, 3);
   treerefresh = new wxButton(treePanel, TreeRefresh, _("Refresh"), wxDefaultPosition, wxSize(60, 25));
   treeCtrlSizer->Add(treerefresh, 0, wxLEFT | wxRIGHT, 3);
   
   treeSizer->Add(treeCtrlSizer, 1, wxALIGN_CENTER_HORIZONTAL, 0);
   
   treePanel->SetSizer(treeSizer);
   
   wxPanel* listPanel = new wxPanel(treelistPanel);
   wxFlexGridSizer *listSizer = new wxFlexGridSizer(2, 1, 0, 0);
   listSizer->AddGrowableCol(0);
   listSizer->AddGrowableRow(0);
   
   list = new wxbListCtrl(listPanel, this, ListCtrl, wxDefaultPosition, wxSize(200,50));
   //treelistSizer->Add(list, 1, wxEXPAND, 10);

   list->SetImageList(imagelist, wxIMAGE_LIST_SMALL);

   wxListItem info;
   info.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_FORMAT);
   info.SetText(_("M"));
   info.SetAlign(wxLIST_FORMAT_CENTER);
   list->InsertColumn(0, info);
   
   info.SetText(_("Filename"));
   info.SetAlign(wxLIST_FORMAT_LEFT);
   list->InsertColumn(1, info);

   info.SetText(_("Size"));
   info.SetAlign(wxLIST_FORMAT_RIGHT);   
   list->InsertColumn(2, info);

   info.SetText(_("Date"));
   info.SetAlign(wxLIST_FORMAT_LEFT);
   list->InsertColumn(3, info);

   info.SetText(_("Perm."));
   info.SetAlign(wxLIST_FORMAT_LEFT);
   list->InsertColumn(4, info);
   
   info.SetText(_("User"));
   info.SetAlign(wxLIST_FORMAT_RIGHT);
   list->InsertColumn(5, info);
   
   info.SetText(_("Group"));
   info.SetAlign(wxLIST_FORMAT_RIGHT);
   list->InsertColumn(6, info);
    
   listSizer->Add(list, 1, wxEXPAND, 0);
   
   wxBoxSizer *listCtrlSizer = new wxBoxSizer(wxHORIZONTAL);
   listadd = new wxButton(listPanel, ListAdd, _("Add"), wxDefaultPosition, wxSize(60, 25));
   listCtrlSizer->Add(listadd, 0, wxLEFT | wxRIGHT, 5);
   listremove = new wxButton(listPanel, ListRemove, _("Remove"), wxDefaultPosition, wxSize(60, 25));
   listCtrlSizer->Add(listremove, 0, wxLEFT | wxRIGHT, 5);
   listrefresh = new wxButton(listPanel, ListRefresh, _("Refresh"), wxDefaultPosition, wxSize(60, 25));
   listCtrlSizer->Add(listrefresh, 0, wxLEFT | wxRIGHT, 5);
   
   listSizer->Add(listCtrlSizer, 1, wxALIGN_CENTER_HORIZONTAL, 0);
   
   listPanel->SetSizer(listSizer);
   
   treelistPanel->SplitVertically(treePanel, listPanel, 210);
   
   treelistPanel->SetMinimumPaneSize(210);
     
   treelistPanel->Show(false);
   
   wxbConfig* config = new wxbConfig();
   config->Add(new wxbConfigParam(_("Job Name"), ConfigJobName, choice, 0, elist));
   config->Add(new wxbConfigParam(_("Client"), ConfigClient, choice, 0, elist));
   config->Add(new wxbConfigParam(_("Fileset"), ConfigFileset, choice, 0, elist));
   config->Add(new wxbConfigParam(_("Pool"), ConfigPool, choice, 0, elist));
   config->Add(new wxbConfigParam(_("Storage"), ConfigStorage, choice, 0, elist));
   config->Add(new wxbConfigParam(_("Before"), ConfigWhen, choice, 0, elist));
   
   configPanel = new wxbConfigPanel(this, config, _("Please configure parameters concerning files to restore :"), RestoreStart, RestoreCancel, -1);
   
   configPanel->Show(true);
   configPanel->Enable(false);
   
   config = new wxbConfig();
   config->Add(new wxbConfigParam(_("Job Name"), -1, text, wxT("")));
   config->Add(new wxbConfigParam(_("Bootstrap"), -1, text, wxT("")));
   config->Add(new wxbConfigParam(_("Where"), ConfigWhere, modifiableText, wxT("")));
   wxString erlist[] = {_("always"), _("if newer"), _("if older"), _("never")};
   config->Add(new wxbConfigParam(_("Replace"), ConfigReplace, choice, 4, erlist));
   config->Add(new wxbConfigParam(_("Fileset"), ConfigFileset, choice, 0, erlist));
   config->Add(new wxbConfigParam(_("Client"), ConfigClient, choice, 0, erlist));
   config->Add(new wxbConfigParam(_("Storage"), ConfigStorage, choice, 0, erlist));
   config->Add(new wxbConfigParam(_("When"), ConfigWhen, modifiableText, wxT("")));
   config->Add(new wxbConfigParam(_("Priority"), ConfigPriority, modifiableText, wxT("")));
   
   restorePanel = new wxbConfigPanel(this, config, _("Please configure parameters concerning files restoration :"), ConfigOk, ConfigCancel, ConfigApply);
    
   restorePanel->Show(false);
   
   centerSizer = new wxBoxSizer(wxHORIZONTAL);
   //centerSizer->Add(treelistPanel, 1, wxEXPAND | wxADJUST_MINSIZE);
      
   mainSizer->Add(centerSizer, 1, wxEXPAND, 10);

   gauge = new wxGauge(this, -1, 1, wxDefaultPosition, wxSize(200,20));

   mainSizer->Add(gauge, 1, wxEXPAND, 5);
   gauge->SetValue(0);
   gauge->Enable(false);

   SetSizer(mainSizer);
   mainSizer->SetSizeHints(this);

   SetStatus(disabled);

   for (int i = 0; i < 7; i++) {
      list->SetColumnWidth(i, 70);
   }

   SetCursor(*wxSTANDARD_CURSOR);

   markWhenCommandDone = false;
   
   cancelled = 0;
}

/*
 *  wxbRestorePanel destructor
 */
wxbRestorePanel::~wxbRestorePanel() 
{
   delete imagelist;
}

/*----------------------------------------------------------------------------
   wxbPanel overloadings
  ----------------------------------------------------------------------------*/

wxString wxbRestorePanel::GetTitle() 
{
   return _("Restore");
}

void wxbRestorePanel::EnablePanel(bool enable) 
{
   if (enable) {
      if (status == disabled) {
         SetStatus(activable);
      }
   } else {
      SetStatus(disabled);
   }
}

/*----------------------------------------------------------------------------
   Commands called by events handler
  ----------------------------------------------------------------------------*/

/* The main button has been clicked */
void wxbRestorePanel::CmdStart() 
{
   unsigned int i;
   if (status == activable) {
      wxbMainFrame::GetInstance()->SetStatusText(_("Getting parameters list."));
      wxbDataTokenizer* dt = wxbUtils::WaitForEnd(wxT(".clients\n"), true, false);
      wxString str;

      configPanel->ClearRowChoices(_("Client"));
      restorePanel->ClearRowChoices(_("Client"));
      
      if (dt->GetCount() == 0) {
         wxbMainFrame::GetInstance()->SetStatusText(_("Error : no clients returned by the director."));
         return;
      }
      
      for (i = 0; i < dt->GetCount(); i++) {
         str = (*dt)[i];
         str.RemoveLast();
         configPanel->AddRowChoice(_("Client"), str);
         restorePanel->AddRowChoice(_("Client"), str);
      }
          
      delete dt;
      
      if (cancelled) {
         cancelled = 2;
         return;
      }
      
      dt = wxbUtils::WaitForEnd(wxT(".filesets\n"), true, false);
      
      configPanel->ClearRowChoices(_("Fileset"));
      restorePanel->ClearRowChoices(_("Fileset"));
    
      if (dt->GetCount() == 0) {
         wxbMainFrame::GetInstance()->SetStatusText(_("Error : no filesets returned by the director."));
         return;
      }
      
      for (i = 0; i < dt->GetCount(); i++) {
         str = (*dt)[i];
         str.RemoveLast();
         configPanel->AddRowChoice(_("Fileset"), str);
         restorePanel->AddRowChoice(_("Fileset"), str);
      }
      
      delete dt;
      
      if (cancelled) {
         cancelled = 2;
         return;
      }
      
      dt = wxbUtils::WaitForEnd(wxT(".storage\n"), true, false);
    
      configPanel->ClearRowChoices(_("Storage"));
      restorePanel->ClearRowChoices(_("Storage"));
    
      if (dt->GetCount() == 0) {
         wxbMainFrame::GetInstance()->SetStatusText(_("Error : no storage returned by the director."));
         return;
      }
      
      for (i = 0; i < dt->GetCount(); i++) {
         str = (*dt)[i];
         str.RemoveLast();
         configPanel->AddRowChoice(_("Storage"), str);
         restorePanel->AddRowChoice(_("Storage"), str);
      }
      
      delete dt;
      
      if (cancelled) {
         cancelled = 2;
         return;
      }
      
      dt = wxbUtils::WaitForEnd(wxT(".jobs\n"), true, false);
    
      configPanel->ClearRowChoices(_("Job Name"));
    
      if (dt->GetCount() == 0) {
         wxbMainFrame::GetInstance()->SetStatusText(_("Error : no jobs returned by the director."));
         return;
      }
      
      for (i = 0; i < dt->GetCount(); i++) {
         str = (*dt)[i];
         str.RemoveLast();
         configPanel->AddRowChoice(_("Job Name"), str);
      }
      
      configPanel->SetRowString(_("Job Name"), _("RestoreFiles"));
      
      delete dt;
      
      if (cancelled) {
         cancelled = 2;
         return;
      }
      
      dt = wxbUtils::WaitForEnd(wxT(".pools\n"), true, false);
    
      configPanel->ClearRowChoices(_("Pool"));
    
      if (dt->GetCount() == 0) {
         wxbMainFrame::GetInstance()->SetStatusText(_("Error : no jobs returned by the director."));
         return;
      }
      
      for (i = 0; i < dt->GetCount(); i++) {
         str = (*dt)[i];
         str.RemoveLast();
         configPanel->AddRowChoice(_("Pool"), str);
      }
         
      delete dt; 
      
      if (cancelled) {
         cancelled = 2;
         return;
      }

      SetStatus(entered);

      UpdateFirstConfig();
           
      wxbMainFrame::GetInstance()->SetStatusText(_("Please configure your restore parameters."));
   }
   else if (status == entered) {
#ifdef xxx
      if (clientChoice->GetStringSelection().Length() < 1) {
         wxbMainFrame::GetInstance()->SetStatusText(_("Please select a client."));
         return;
      }
      if (jobChoice->GetStringSelection().Length() < 1) {
         wxbMainFrame::GetInstance()->SetStatusText(_("Please select a restore date."));
         return;
      }
#endif

      wxbMainFrame::GetInstance()->SetStatusText(_("Building restore tree..."));
      
      SetStatus(choosing);
      
      wxbTableParser* tableparser = new wxbTableParser();
      wxbDataTokenizer* dt = new wxbDataTokenizer(false);
      
/*
 * The following line was removed from  ::GetInstance below because
 *  it does not work with multiple pools -- KES 5Oct05 see bug #433  
 *       wxT("\" pool=\"") << configPanel->GetRowString(wxT("Pool")) <<
 */
      wxbMainFrame::GetInstance()->Send(wxString(wxT("restore")) <<
         wxT(" client=\"") << configPanel->GetRowString(wxT("Client")) <<
         wxT("\" fileset=\"") << configPanel->GetRowString(wxT("Fileset")) <<
         wxT("\" storage=\"") << configPanel->GetRowString(wxT("Storage")) <<
         wxT("\" before=\"") << configPanel->GetRowString(wxT("Before")) <<
         wxT("\" select\n"));

#ifdef xxx
      wxbUtils::WaitForPrompt("6\n");
      WaitForEnd();

      wxbPromptParser *pp = wxbUtils::WaitForPrompt(wxString() << configPanel->GetRowString(wxT("Before")) << "\n", true);

      int client = pp->getChoices()->Index(configPanel->GetRowString(wxT("Client")));
      if (client == wxNOT_FOUND) {
         wxbMainFrame::GetInstance()->SetStatusText("Failed to find the selected client.");
         return;
      }
      delete pp;
      
      wxbMainFrame::GetInstance()->Send(wxString() << configPanel->GetRowString(wxT("Before")) << "\n");
#endif
   
      while (!tableparser->hasFinished() && !dt->hasFinished()) {
         wxTheApp->Yield(true);
         wxbUtils::MilliSleep(100);
      }
      
      wxString str;

      if (dt->hasFinished() && !tableparser->hasFinished()) {
         str = wxT("");
         if (dt->GetCount() > 1) {
            str = (*dt)[dt->GetCount()-2];
            str.RemoveLast();
         }
         wxbMainFrame::GetInstance()->SetStatusText(wxString(_("Error while starting restore: ")) << str);
         delete dt;
         delete tableparser;
         SetStatus(finished);
         return;
      }
           
      int tot = 0;
      long l;
      
      for (i = 0; i < tableparser->GetCount(); i++) {
         str = (*tableparser)[i][2];
         str.Replace(wxT(","), wxT(""));
         if (str.ToLong(&l)) {
            tot += l;
         }
      }
           
      gauge->SetValue(0);
      gauge->SetRange(tot);
      
#ifdef xxx
      wxbMainFrame::GetInstance()->Print(
               wxString("[") << tot << "]", CS_DEBUG);
#endif
      
      wxDateTime base = wxDateTime::Now();
      wxDateTime newdate;
      int done = 0;
      int willdo = 0;
      unsigned int lastindex = 0;
      
      int var = 0;
      
      int i1, i2;
      
      while (true) {
         newdate = wxDateTime::Now();
         if (newdate.Subtract(base).GetMilliseconds() > 10 ) {
            base = newdate;
            for (; lastindex < dt->GetCount(); lastindex++) {
               if (((i1 = (*dt)[lastindex].Find(wxT("Building directory tree for JobId "))) >= 0) && 
                     ((i2 = (*dt)[lastindex].Find(wxT(" ..."))) > 0)) {
                  str = (*dt)[lastindex].Mid(i1+34, i2-(i1+34));
                  for (i = 0; i < tableparser->GetCount(); i++) {
                     if (str == (*tableparser)[i][0]) {
                        str = (*tableparser)[i][2];
                        str.Replace(wxT(","), wxT(""));
                        if (str.ToLong(&l)) {
                           done += willdo;
                           willdo += l;
                           var = (willdo-done)/50;
                           gauge->SetValue(done);
                           wxTheApp->Yield(true);
                        }
                        break;
                     }
                  }
               }
               else if ((*dt)[lastindex] == wxT("+")) {
                  gauge->SetValue(gauge->GetValue()+var);
                  wxTheApp->Yield(true);
               }
            }
            
                       
            if (dt->hasFinished()) {
               break;
            }
         }
         wxTheApp->Yield(true);
         wxbUtils::MilliSleep(1);
      }

      gauge->SetValue(tot);
      wxTheApp->Yield(true);
      gauge->SetValue(0);
      
      delete dt;
      delete tableparser;

      if (cancelled) {
         cancelled = 2;
         return;
      }

      wxbUtils::WaitForEnd(wxT("unmark *\n"));
      wxTreeItemId root = tree->AddRoot(configPanel->GetRowString(_("Client")), -1, -1, new wxbTreeItemData(wxT("/"), configPanel->GetRowString(_("Client")), 0));
      currentTreeItem = root;
      tree->Refresh();
      tree->SelectItem(root);
      CmdList(root);
      wxbMainFrame::GetInstance()->SetStatusText(_("Right click on a file or on a directory, or double-click on its mark to add it to the restore list."));
      tree->Expand(root);
   }
   else if (status == choosing) {
      EnableConfig(false);
      
      totfilemessages = 0;
      wxbDataTokenizer* dt;
           
      int j;
      
      dt = new wxbDataTokenizer(true);
      wxbPromptParser* promptparser = wxbUtils::WaitForPrompt(wxT("done\n"), true);

      while (!promptparser->getChoices() || (promptparser->getChoices()->Index(wxT("mod")) < 0)) {
         wxbMainFrame::GetInstance()->Print(_("Unexpected question has been received.\n"), CS_DEBUG);
         
         wxString message;
         if (promptparser->getIntroString() != wxT("")) {
            message << promptparser->getIntroString() << wxT("\n");
         }
         message << promptparser->getQuestionString();
         
         if (promptparser->getChoices()) {
            wxString *choices = new wxString[promptparser->getChoices()->GetCount()];
            int *numbers = new int[promptparser->getChoices()->GetCount()];
            int n = 0;
            
            for (unsigned int i = 0; i < promptparser->getChoices()->GetCount(); i++) {
               if ((*promptparser->getChoices())[i] != wxT("")) {
                  choices[n] = (*promptparser->getChoices())[i];
                  numbers[n] = i;
                  n++;
               }
            }
            
            int res = ::wxGetSingleChoiceIndex(message,
               _("bwx-console: unexpected restore question."), n, choices, this);
            if (res == -1) {
               delete promptparser;
               promptparser = wxbUtils::WaitForPrompt(wxT(".\n"), true);
            }
            else {
               if (promptparser->isNumericalChoice()) {
                  delete promptparser;
                  promptparser = wxbUtils::WaitForPrompt(wxString() << numbers[res] << wxT("\n"), true);
               }
               else {
                  delete promptparser;
                  promptparser = wxbUtils::WaitForPrompt(wxString() << choices[res] << wxT("\n"), true);
               }
            }
            delete[] choices;
            delete[] numbers;
         }
         else {
            delete promptparser;
            
            promptparser = wxbUtils::WaitForPrompt(::wxGetTextFromUser(message,
               _("bwx-console: unexpected restore question."),
               wxT(""), this) + wxT("\n"));
         }
      }
      printf("promptparser->getChoices()=%ld", (long)promptparser->getChoices());
      
      delete promptparser;

      SetStatus(configuring);

      for (i = 0; i < dt->GetCount(); i++) {
         if ((j = (*dt)[i].Find(_(" files selected to be restored."))) > -1) {
            (*dt)[i].Mid(0, j).ToLong(&totfilemessages);
            break;
         }

         if ((j = (*dt)[i].Find(_(" file selected to be restored."))) > -1) {
            (*dt)[i].Mid(0, j).ToLong(&totfilemessages);
            break;
         }
      }
      
      wxbMainFrame::GetInstance()->SetStatusText(
         wxString::Format(_("Please configure your restore (%ld files selected to be restored)..."), totfilemessages));
      
      UpdateSecondConfig(dt);
      
      delete dt;
      
      EnableConfig(true);
      restorePanel->EnableApply(false);

      if (totfilemessages == 0) {
         wxbMainFrame::GetInstance()->Print(_("Restore failed : no file selected.\n"), CS_DEBUG);
         wxbMainFrame::GetInstance()->SetStatusText(_("Restore failed : no file selected."));
         SetStatus(finished);
         return;
      }
   }
   else if (status == configuring) {
      cancel->Enable(false);
      jobid = wxT("");
      EnableConfig(false);
    
      wxbMainFrame::GetInstance()->SetStatusText(_("Restoring, please wait..."));
    
      wxbDataTokenizer* dt;
    
      SetStatus(restoring);
      dt = wxbUtils::WaitForEnd(wxT("yes\n"), true);

      gauge->SetValue(0);
      gauge->SetRange(totfilemessages);

      int j;
            
      for (i = 0; i < dt->GetCount(); i++) {
         if ((j = (*dt)[i].Find(_("Job queued. JobId="))) > -1) {
            jobid = (*dt)[i].Mid(j+19);
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore queued, jobid=") + jobid);
            break;
         }

         if ((j = (*dt)[i].Find(_("Job failed."))) > -1) {
            wxbMainFrame::GetInstance()->Print(_("Restore failed, please look at messages.\n"), CS_DEBUG);
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore failed, please look at messages in console."));
            return;
         }
      }
      
      if (jobid == wxT("")) {
         wxbMainFrame::GetInstance()->Print(_("Failed to retrieve jobid.\n"), CS_DEBUG);
         wxbMainFrame::GetInstance()->SetStatusText(_("Failed to retrieve jobid.\n"));
         return;         
      }

      wxDateTime currenttime;
      
      dt = wxbUtils::WaitForEnd(wxT("time\n"), true);
      wxStringTokenizer ttkz((*dt)[0], wxT(" "), wxTOKEN_STRTOK);
      if ((currenttime.ParseDate(ttkz.GetNextToken()) == NULL) || // Date
           (currenttime.ParseTime(ttkz.GetNextToken()) == NULL)) { // Time
         currenttime.SetYear(1990); // If parsing fails, set currenttime to a dummy date
      }
      else {
         currenttime -= wxTimeSpan::Seconds(30); //Adding a 30" tolerance
      }
      delete dt;
    
      wxDateTime scheduledtime;
      wxStringTokenizer stkz(restorePanel->GetRowString(_("When")), wxT(" "), wxTOKEN_STRTOK);
      
      if ((scheduledtime.ParseDate(stkz.GetNextToken()) == NULL) || // Date
           (scheduledtime.ParseTime(stkz.GetNextToken()) == NULL)) { // Time
         scheduledtime.SetYear(2090); // If parsing fails, set scheduledtime to a dummy date
      }

      if (scheduledtime.Subtract(currenttime).IsLongerThan(wxTimeSpan::Seconds(150))) {
         wxbMainFrame::GetInstance()->Print(_("Restore is scheduled to run. bwx-console will not wait for its completion.\n"), CS_DEBUG);
         wxbMainFrame::GetInstance()->SetStatusText(_("Restore is scheduled to run. bwx-console will not wait for its completion."));
         SetStatus(finished);
         return;
      }

      wxString cmd = wxString(wxT("list jobid=")) + jobid;

      wxbTableParser* tableparser;
      
      long filemessages = 0;
      
      bool ended = false;
      bool waitforever = false;
      
      char status = '?';

      wxStopWatch sw;
      
      wxbUtils::WaitForEnd(wxT("autodisplay off\n"));
      wxbUtils::WaitForEnd(wxT("gui on\n"));
      while (true) {
         tableparser = wxbUtils::CreateAndWaitForParser(cmd);
         ended = false;
         status = (*tableparser)[0][7].GetChar(0);
         switch (status) {
         case JS_Created:
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore job created, but not yet running."));
            waitforever = false;
            break;
         case JS_Running:
            wxbMainFrame::GetInstance()->SetStatusText(
               wxString::Format(_("Restore job running, please wait (%ld of %ld files restored)..."), filemessages, totfilemessages));
            waitforever = true;
            break;
         case JS_Terminated:
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore job terminated successfully."));
            wxbMainFrame::GetInstance()->Print(_("Restore job terminated successfully.\n"), CS_DEBUG);
            waitforever = false;
            ended = true;
            break;
         case JS_ErrorTerminated:
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore job terminated in error, see messages in console."));
            wxbMainFrame::GetInstance()->Print(_("Restore job terminated in error, see messages.\n"), CS_DEBUG);
            waitforever = false;
            ended = true;
            break;
         case JS_Error:
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore job reported a non-fatal error."));
            waitforever = false;
            break;
         case JS_FatalError:
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore job reported a fatal error."));
            waitforever = false;
            ended = true;
            break;
         case JS_Canceled:
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore job cancelled by user."));
            wxbMainFrame::GetInstance()->Print(_("Restore job cancelled by user.\n"), CS_DEBUG);
            waitforever = false;
            ended = true;
            break;
         case JS_WaitFD:
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore job is waiting on File daemon."));
            waitforever = false;
            break;
         case JS_WaitMedia:
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore job is waiting for new media."));
            waitforever = false;
            break;
         case JS_WaitStoreRes:
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore job is waiting for storage resource."));
            waitforever = false;
            break;
         case JS_WaitJobRes:
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore job is waiting for job resource."));
            waitforever = false;
            break;
         case JS_WaitClientRes:
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore job is waiting for Client resource."));
            waitforever = false;
            break;
         case JS_WaitMaxJobs:
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore job is waiting for maximum jobs."));
            waitforever = false;
            break;
         case JS_WaitStartTime:
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore job is waiting for start time."));
            waitforever = false;
            break;
         case JS_WaitPriority:
            wxbMainFrame::GetInstance()->SetStatusText(_("Restore job is waiting for higher priority jobs to finish."));
            waitforever = false;
            break;
         }
         delete tableparser;
         
         dt = wxbUtils::WaitForEnd(wxT(".messages\n"), true);
                  
         for (unsigned int i = 0; i < dt->GetCount(); i++) {
            wxStringTokenizer tkz((*dt)[i], wxT(" "), wxTOKEN_STRTOK);
   
            wxDateTime datetime;
   
            //   Date    Time   name:   perm      ?   user   grp      size    date     time
            //04-Apr-2004 17:19 Tom-fd: -rwx------   1 nicolas  None     514967 2004-03-20 20:03:42  filename
   
            if (datetime.ParseDate(tkz.GetNextToken()) != NULL) { // Date
               if (datetime.ParseTime(tkz.GetNextToken()) != NULL) { // Time
                  if (tkz.GetNextToken().Last() == ':') { // name:
                  tkz.GetNextToken(); // perm
                  tkz.GetNextToken(); // ?
                  tkz.GetNextToken(); // user
                  tkz.GetNextToken(); // grp
                  tkz.GetNextToken(); // size
                  if (datetime.ParseDate(tkz.GetNextToken()) != NULL) { //date
                        if (datetime.ParseTime(tkz.GetNextToken()) != NULL) { //time
                           filemessages++;
                           //wxbMainFrame::GetInstance()->Print(wxString("(") << filemessages << ")", CS_DEBUG);
                           gauge->SetValue(filemessages);
                        }
                     }
                  }
               }
            }
         }
         
         delete dt;
         
         wxStopWatch sw2;
         while (sw2.Time() < 10000) {  
            wxTheApp->Yield(true);
            wxbUtils::MilliSleep(100);
         }
         
         if (ended) {
            break;
         }
         
         if ((!waitforever) && (sw.Time() > 60000)) {
            wxbMainFrame::GetInstance()->Print(_("The restore job has not been started within one minute, bwx-console will not wait for its completion anymore.\n"), CS_DEBUG);
            wxbMainFrame::GetInstance()->SetStatusText(_("The restore job has not been started within one minute, bwx-console will not wait for its completion anymore."));
            break;
         }
      }
      wxbUtils::WaitForEnd(wxT("autodisplay on\n"));
      wxbUtils::WaitForEnd(wxT(".messages\n"));

      gauge->SetValue(totfilemessages);

      if (status == JS_Terminated) {
         wxbMainFrame::GetInstance()->Print(_("Restore done successfully.\n"), CS_DEBUG);
         wxbMainFrame::GetInstance()->SetStatusText(_("Restore done successfully."));
      }
      SetStatus(finished);
   }
}

/* The cancel button has been clicked */
void wxbRestorePanel::CmdCancel() {
   cancelled = 1;
   
   if (status == restoring) {
      if (jobid != wxT("")) {
         wxbMainFrame::GetInstance()->Send(wxString(wxT("cancel job=")) << jobid << wxT("\n"));
      }
      cancel->Enable(true);
      return;
   }
   
   wxStopWatch sw;
   while ((IsWorking()) && (cancelled != 2)) {
      wxTheApp->Yield(true);
      wxbUtils::MilliSleep(100);
      if (sw.Time() > 30000) { /* 30 seconds timeout */
         if (status == choosing) {
            wxbMainFrame::GetInstance()->Send(wxT("quit\n"));
         }
         else if (status == configuring) {
            wxbMainFrame::GetInstance()->Send(wxT("no\n"));
         }
         else if (status == restoring) {
            
         }
         SetStatus(finished);
         wxbUtils::MilliSleep(1000);
         return;
      }
   }
   
   switch (status) {
   case choosing:
      wxbMainFrame::GetInstance()->Send(wxT("quit\n"));
      break;
   case configuring:
      wxbMainFrame::GetInstance()->Send(wxT("no\n"));
      break;
   default:
      break;
   }
   wxbUtils::MilliSleep(1000);
   SetStatus(finished);
}

/* Apply configuration changes */

/*   1: Level (not appropriate)
 *   2: Storage (yes)
 *   3: Job (no)
 *   4: FileSet (yes)
 *   5: Client (yes)
 *   6: When (yes : "Please enter desired start time as YYYY-MM-DD HH:MM:SS (return for now):")
 *   7: Priority (yes : "Enter new Priority: (positive integer)")
 *   8: Bootstrap (?)
 *   9: Where (yes : "Please enter path prefix for restore (/ for none):")
 *  10: Replace (yes : "Replace:\n 1: always\n 2: ifnewer\n 3: ifolder\n 4: never\n 
 *         Select replace option (1-4):")
 *  11: JobId (no)
 */

void wxbRestorePanel::CmdConfigApply() 
{
   if (cfgUpdated == 0) return;
   
   wxbMainFrame::GetInstance()->SetStatusText(_("Applying restore configuration changes..."));
   
   EnableConfig(false);
   
   wxbDataTokenizer* dt = NULL;
   
   bool failed = false;
   
   while (cfgUpdated > 0) {
      if (cancelled) {
         cancelled = 2;
         return;
      }
      wxString def; //String to send if can't use our data
      if ((cfgUpdated >> ConfigWhere) & 1) {
         wxbUtils::WaitForPrompt(wxT("mod\n")); /* TODO: check results */
         wxbUtils::WaitForPrompt(wxT("9\n"));
         dt = new wxbDataTokenizer(true);
         wxbUtils::WaitForPrompt(restorePanel->GetRowString(_("Where")) + wxT("\n"));
         def = wxT("/tmp");
         cfgUpdated = cfgUpdated & (~(1 << ConfigWhere));
      }
      else if ((cfgUpdated >> ConfigReplace) & 1) {
         wxbUtils::WaitForPrompt(wxT("mod\n")); /* TODO: check results */
         wxbUtils::WaitForPrompt(wxT("11\n"));
         dt = new wxbDataTokenizer(true);
         wxbUtils::WaitForPrompt(wxString() << (restorePanel->GetRowSelection(_("Replace"))+1) << wxT("\n"));
         def = wxT("1");
         cfgUpdated = cfgUpdated & (~(1 << ConfigReplace));
      }
      else if ((cfgUpdated >> ConfigWhen) & 1) {
         wxbUtils::WaitForPrompt(wxT("mod\n")); /* TODO: check results */
         wxbUtils::WaitForPrompt(wxT("6\n"));
         dt = new wxbDataTokenizer(true);
         wxbUtils::WaitForPrompt(restorePanel->GetRowString(wxT("When")) + wxT("\n"));
         def = wxT("");
         cfgUpdated = cfgUpdated & (~(1 << ConfigWhen));
      }
      else if ((cfgUpdated >> ConfigPriority) & 1) {
         wxbUtils::WaitForPrompt(wxT("mod\n")); /* TODO: check results */
         wxbUtils::WaitForPrompt(wxT("7\n"));
         dt = new wxbDataTokenizer(true);
         wxbUtils::WaitForPrompt(restorePanel->GetRowString(_("Priority")) + wxT("\n"));
         def = wxT("10");
         cfgUpdated = cfgUpdated & (~(1 << ConfigPriority));
      }
      else if ((cfgUpdated >> ConfigClient) & 1) {
         if (restorePanel->GetRowCount(_("Client")) > 1) {
            wxbUtils::WaitForPrompt(wxT("mod\n")); /* TODO: check results */
            wxbPromptParser *pp = wxbUtils::WaitForPrompt(wxT("5\n"), true);
            int client = pp->getChoices()->Index(restorePanel->GetRowString(_("Client")));
            if (client == wxNOT_FOUND) {
               wxbMainFrame::GetInstance()->SetStatusText(_("Failed to find the selected client."));
               failed = true;
               client = 1;
            }
            delete pp;
            dt = new wxbDataTokenizer(true);
            wxbUtils::WaitForPrompt(wxString() << client << wxT("\n"));
            def = wxT("1");
            cfgUpdated = cfgUpdated & (~(1 << ConfigClient));
         } else {
            cfgUpdated = cfgUpdated & (~(1 << ConfigClient));
            continue;
         }
      }
      else if ((cfgUpdated >> ConfigFileset) & 1) {
         if (restorePanel->GetRowCount(_("Fileset")) > 1) {
            wxbUtils::WaitForPrompt(wxT("mod\n")); /* TODO: check results */
            wxbPromptParser *pp = wxbUtils::WaitForPrompt(wxT("4\n"), true);
            int fileset = pp->getChoices()->Index(restorePanel->GetRowString(_("Fileset")));
            if (fileset == wxNOT_FOUND) {
               wxbMainFrame::GetInstance()->SetStatusText(_("Failed to find the selected fileset."));
               failed = true;
               fileset = 1;
            }
            delete pp;
            dt = new wxbDataTokenizer(true);
            wxbUtils::WaitForPrompt(wxString() << fileset << wxT("\n"));
            def = wxT("1");
            cfgUpdated = cfgUpdated & (~(1 << ConfigFileset));
         } else {
            cfgUpdated = cfgUpdated & (~(1 << ConfigFileset));
            continue;
         }
      }
      else if ((cfgUpdated >> ConfigStorage) & 1) {
         if (restorePanel->GetRowCount(_("Storage")) > 1) {
            wxbUtils::WaitForPrompt(wxT("mod\n")); /* TODO: check results */
            wxbPromptParser *pp = wxbUtils::WaitForPrompt(wxT("2\n"), true);
            int storage = pp->getChoices()->Index(restorePanel->GetRowString(_("Storage")));
            if (storage == wxNOT_FOUND) {
               wxbMainFrame::GetInstance()->SetStatusText(_("Failed to find the selected storage."));
               failed = true;
               storage = 1;
            }
            delete pp;
            dt = new wxbDataTokenizer(true);
            wxbUtils::WaitForPrompt(wxString() << storage << wxT("\n"));
            def = wxT("1");
            cfgUpdated = cfgUpdated & (~(1 << ConfigStorage));
         } else {
            cfgUpdated = cfgUpdated & (~(1 << ConfigStorage));
            continue;
         }
      }
      else {
         cfgUpdated = 0;
         break;
      }
                 
      unsigned int i;
      for (i = 0; i < dt->GetCount(); i++) {
         if ((*dt)[i].Find(_("Run Restore job")) == 0) {
            break;
         }
      }
      
      if (i != 0 && i == dt->GetCount()) {
         delete dt;   
         dt = wxbUtils::WaitForEnd(def + wxT("\n"), true);
         failed = true;
      }
   }
   UpdateSecondConfig(dt); /* TODO: Check result */
   
   EnableConfig(true);
   restorePanel->EnableApply(false);

   if (!failed) {
      wxbMainFrame::GetInstance()->SetStatusText(_("Restore configuration changes were applied."));
   }

   delete dt;
}

/* Cancel restore */
void wxbRestorePanel::CmdConfigCancel() {
   wxbUtils::WaitForEnd(wxT("no\n"));
   wxbMainFrame::GetInstance()->Print(_("Restore cancelled.\n"), CS_DEBUG);
   wxbMainFrame::GetInstance()->SetStatusText(_("Restore cancelled."));
   SetStatus(finished);
}

/* List jobs for a specified client and fileset */
void wxbRestorePanel::CmdListJobs() {
   if (status == entered) {
      configPanel->ClearRowChoices(_("Before"));
      /*wxbUtils::WaitForPrompt("query\n");
      wxbUtils::WaitForPrompt("6\n");*/
      wxbTableParser* tableparser = new wxbTableParser(false);
      wxbDataTokenizer* dt = wxbUtils::WaitForEnd(
         wxString(wxT(".backups client=\"")) + configPanel->GetRowString(_("Client")) + 
                  wxT("\" fileset=\"") + configPanel->GetRowString(_("Fileset")) + wxT("\"\n"), true);

      while (!tableparser->hasFinished()) {
         wxTheApp->Yield(true);
         wxbUtils::MilliSleep(100);
      }
         
      if (!tableparser->GetCount() == 0) {
         for (unsigned int i = 0; i < dt->Count(); i++) {
            if ((*dt)[i].Find(_("No results to list.")) == 0) {
               configPanel->AddRowChoice(_("Before"),
                  _("No backup found for this client."));
               configPanel->SetRowSelection(_("Before"), 0);
               configPanel->EnableApply(true); // Enabling the not existing apply button disables the ok button.
               delete tableparser;
               delete dt;
               return;
            }
            else if (((*dt)[i].Find(_("ERROR")) > -1) || 
                  ((*dt)[i].Find(_("Query failed")) > -1)) {
               configPanel->AddRowChoice(_("Before"),
                  _("Cannot get previous backups list, see console."));
               configPanel->SetRowSelection(_("Before"), 0);
               configPanel->EnableApply(true); // Enabling the not existing apply button disables the ok button.
               delete tableparser;
               delete dt;
               return;
            }
         }
      }
      
      delete dt;

      wxDateTime lastdatetime = (time_t) 0;
      for (int i = tableparser->GetCount()-1; i > -1; i--) {
         wxString str = (*tableparser)[i][3];
         wxDateTime datetime;
         const wxChar* chr;
         if ( ( (chr = datetime.ParseDate(str.GetData()) ) != NULL ) && ( datetime.ParseTime(++chr) != NULL ) && ! lastdatetime.IsEqualTo(datetime) ) {
            lastdatetime = datetime;
            datetime += wxTimeSpan::Seconds(1);
            configPanel->AddRowChoice(_("Before"),
               datetime.Format(wxT("%Y-%m-%d %H:%M:%S")));
         }
      }
           
      delete tableparser;

      configPanel->SetRowSelection(_("Before"), 0);
      configPanel->EnableApply(false); // Disabling the not existing apply button enables the ok button.
   }
}

/* List files and directories for a specified tree item */
void wxbRestorePanel::CmdList(wxTreeItemId item) {
   if (status == choosing) {
      list->DeleteAllItems();

      if (!item.IsOk()) {
         return;
      }
      UpdateTreeItem(item, true, false);
    
      if (list->GetItemCount() >= 1) {
         int firstwidth = list->GetSize().GetWidth(); 
         for (int i = 2; i < 7; i++) {
            list->SetColumnWidth(i, wxLIST_AUTOSIZE);
            firstwidth -= list->GetColumnWidth(i);
         }
       
         list->SetColumnWidth(0, 18);
         firstwidth -= 18;
         list->SetColumnWidth(1, wxLIST_AUTOSIZE);
         if (list->GetColumnWidth(1) < firstwidth) {
            list->SetColumnWidth(1, firstwidth-25);
         }
      }
   }
}

/* Mark a treeitem (directory) or a listitem (file or directory) */
void wxbRestorePanel::CmdMark(wxTreeItemId treeitem, long* listitems, int listsize, int state) {
   if (status == choosing) {
      wxbTreeItemData** itemdata;
      int itemdatasize = 0;
      if (listsize == 0) {
         itemdata = new wxbTreeItemData*[1];
         itemdatasize = 1;
      }
      else {
         itemdata = new wxbTreeItemData*[listsize];
         itemdatasize = listsize;
      }
      
      if (listitems != NULL) {
         for (int i = 0; i < itemdatasize; i++) {
            itemdata[i] = (wxbTreeItemData*)list->GetItemData(listitems[i]);
         }
      }
      else if (treeitem.IsOk()) {
         itemdata[0] = (wxbTreeItemData*)tree->GetItemData(treeitem);
      }
      else {
         delete[] itemdata;
         return;
      }

      if (itemdata[0] == NULL) { //Should never happen
         delete[] itemdata;
         return;
      }

      wxString dir = itemdata[0]->GetPath();
      wxString file;

      if (dir != wxT("/")) {
         if (IsPathSeparator(dir.GetChar(dir.Length()-1))) {
            dir.RemoveLast();
         }

         int i = dir.Find('/', TRUE);
         if (i == -1) {
            file = dir;
            dir = wxT("/");
         }
         else { /* first dir below root */
            file = dir.Mid(i+1);
            dir = dir.Mid(0, i+1);
         }
      }
      else {
         dir = wxT("/");
         file = wxT("*");
      }

      if (state == -1) {
         bool marked = false;
         bool unmarked = false;
         state = 0;
         for (int i = 0; i < itemdatasize; i++) {
            switch(itemdata[i]->GetMarked()) {
            case 0:
               unmarked = true;
               break;
            case 1:
               marked = true;
               break;
            case 2:
               marked = true;
               unmarked = true;
               break;
            default:
               break;
            }
            if (marked && unmarked)
               break;
         }
         if (marked) {
            if (unmarked) {
               state = 1;
            }
            else {
               state = 0;
            }
         }
         else {
            state = 1;
         }
      }

      wxbUtils::WaitForEnd(wxString(wxT("cd \"")) << dir << wxT("\"\n"));
      wxbUtils::WaitForEnd(wxString((state==1) ? wxT("mark") : wxT("unmark")) << wxT(" \"") << file << wxT("\"\n"));

      /* TODO: Check commands results */

      /*if ((dir == "/") && (file == "*")) {
            itemdata->SetMarked((itemdata->GetMarked() == 1) ? 0 : 1);
      }*/

      if (listitems == NULL) { /* tree item state changed */
         SetTreeItemState(treeitem, state);
         /*treeitem = tree->GetSelection();
         UpdateTree(treeitem, true);
         treeitem = tree->GetItemParent(treeitem);*/
      }
      else {
         for (int i = 0; i < itemdatasize; i++) {
            SetListItemState(listitems[i], state);
         }
         listadd->Enable(state == 0);
         listremove->Enable(state == 1);
         /*UpdateTree(treeitem, (tree->GetSelection() == treeitem));
         treeitem = tree->GetItemParent(treeitem);*/
      }

      /*while (treeitem.IsOk()) {
         WaitForList(treeitem, false);
         treeitem = tree->GetItemParent(treeitem);
      }*/
      
      delete[] itemdata;
   }
}

/*----------------------------------------------------------------------------
   General functions
  ----------------------------------------------------------------------------*/

/* Run a dir command, and waits until result is fully received. */
void wxbRestorePanel::UpdateTreeItem(wxTreeItemId item, bool updatelist, bool recurse)
{
//   this->updatelist = updatelist;
   wxbDataTokenizer* dt;

   dt = wxbUtils::WaitForEnd(wxString(wxT("cd \"")) << 
      static_cast<wxbTreeItemData*>(tree->GetItemData(item))
         ->GetPath() << wxT("\"\n"), false);

   /* TODO: check command result */
   
   //delete dt;

   status = listing;

   if (updatelist)
      list->DeleteAllItems();
   dt = wxbUtils::WaitForEnd(wxT(".dir\n"), true);
   
   wxString str;
   
   for (unsigned int i = 0; i < dt->GetCount(); i++) {
      str = (*dt)[i];
      
      if (str.Find(wxT("cwd is:")) == 0) { // Sometimes cd command result "infiltrate" into listings.
         break;
      }

      str.RemoveLast();

      wxbDirEntry entry;
      
      if (!ParseList(str, &entry))
            break;

      wxTreeItemId treeid;

      if (IsPathSeparator(entry.fullname.GetChar(entry.fullname.Length()-1))) {
         wxString itemStr;

#if wxCHECK_VERSION(2, 6, 0)
         wxTreeItemIdValue cookie;
#else
         long cookie;
#endif
         
         treeid = tree->GetFirstChild(item, cookie);

         bool updated = false;

         while (treeid.IsOk()) {
            itemStr = ((wxbTreeItemData*)tree->GetItemData(treeid))->GetName();
            if (entry.filename == itemStr) {
               if (static_cast<wxbTreeItemData*>(tree->GetItemData(treeid))->GetMarked() != entry.marked) {
                  tree->SetItemImage(treeid, entry.marked, wxTreeItemIcon_Normal);
                  tree->SetItemImage(treeid, entry.marked, wxTreeItemIcon_Selected);
                  static_cast<wxbTreeItemData*>(tree->GetItemData(treeid))->SetMarked(entry.marked);
               }
               if ((recurse) && (tree->IsExpanded(treeid))) {
                  UpdateTreeItem(treeid, false, true);
               }
               updated = true;
               break;
            }
            treeid = tree->GetNextChild(item, cookie);
         }

         if (!updated) {
            treeid = tree->AppendItem(item, wxbUtils::ConvertToPrintable(entry.filename), entry.marked, entry.marked, new wxbTreeItemData(entry.fullname, entry.filename, entry.marked));
         }
      }

      if (updatelist) {
         long ind = list->InsertItem(list->GetItemCount(), entry.marked);
         wxbTreeItemData* data = new wxbTreeItemData(entry.fullname, entry.filename, entry.marked, ind);
         data->SetId(treeid);
         list->SetItemData(ind, (long)data);
         list->SetItem(ind, 1, wxbUtils::ConvertToPrintable(entry.filename));
         list->SetItem(ind, 2, entry.size);
         list->SetItem(ind, 3, entry.date);
         list->SetItem(ind, 4, entry.perm);
         list->SetItem(ind, 5, entry.user);
         list->SetItem(ind, 6, entry.group);
      }
   }
   
   delete dt;
   
   tree->Refresh();
   status = choosing;
}

/* Parse .dir command results, returns true if the result has been stored in entry, false otherwise. */
int wxbRestorePanel::ParseList(wxString line, wxbDirEntry* entry) 
{
   /* See ls_output in dird/ua_tree.c */
   //-rw-r-----,1,root,root,41575,2005-10-18 18:21:36, ,/usr/var/bacula/working/bacula.sql

   wxStringTokenizer tkz(line, wxT(","));
   
   if (!tkz.HasMoreTokens())
      return false;
   entry->perm = tkz.GetNextToken();
   
   if (!tkz.HasMoreTokens())
      return false;
   entry->nlink = tkz.GetNextToken();
   
   if (!tkz.HasMoreTokens())
      return false;
   entry->user = tkz.GetNextToken();
   
   if (!tkz.HasMoreTokens())
      return false;
   entry->group = tkz.GetNextToken();
   
   if (!tkz.HasMoreTokens())
      return false;
   entry->size = tkz.GetNextToken();
   
   if (!tkz.HasMoreTokens())
      return false;
   entry->date = tkz.GetNextToken();
   
   if (!tkz.HasMoreTokens())
      return false;
   wxString marked = tkz.GetNextToken();
   if (marked == wxT("*")) {
      entry->marked = 1;
   }
   else if (marked == wxT("+")) {
      entry->marked = 2;
   }
   else {
      entry->marked = 0;
   }
   
   if (!tkz.HasMoreTokens())
      return false;
   entry->fullname = tkz.GetString();
   
   /* Get only the filename (cut path by finding the last '/') */
   if (IsPathSeparator(entry->fullname.GetChar(entry->fullname.Length()-1))) {
      wxString tmp = entry->fullname;
      tmp.RemoveLast();
      entry->filename = entry->fullname.Mid(tmp.Find('/', true)+1);
   }
   else {
      entry->filename = entry->fullname.Mid(entry->fullname.Find('/', true)+1);
   }

   return true;
}

/* Sets a list item state, and update its parents and children if it is a directory */
void wxbRestorePanel::SetListItemState(long listitem, int newstate) 
{
   wxbTreeItemData* itemdata = (wxbTreeItemData*)list->GetItemData(listitem);
   
   wxTreeItemId treeitem;
   
   itemdata->SetMarked(newstate);
   list->SetItemImage(listitem, newstate, 0); /* TODO: Find what these ints are for */
   list->SetItemImage(listitem, newstate, 1);
      
   if ((treeitem = itemdata->GetId()).IsOk()) {
      SetTreeItemState(treeitem, newstate);
   }
   else {
      UpdateTreeItemState(tree->GetSelection());
   }
}

/* Sets a tree item state, and update its children, parents and list (if necessary) */
void wxbRestorePanel::SetTreeItemState(wxTreeItemId item, int newstate) {
#if wxCHECK_VERSION(2, 6, 0)
   wxTreeItemIdValue cookie;
#else
   long cookie;
#endif
   wxTreeItemId currentChild = tree->GetFirstChild(item, cookie);

   wxbTreeItemData* itemdata;

   while (currentChild.IsOk()) {
      itemdata = (wxbTreeItemData*)tree->GetItemData(currentChild);
      int state = itemdata->GetMarked();
      
      if (state != newstate) {
         itemdata->SetMarked(newstate);
         tree->SetItemImage(currentChild, newstate, wxTreeItemIcon_Normal);
         tree->SetItemImage(currentChild, newstate, wxTreeItemIcon_Selected);
      }
      
      currentChild = tree->GetNextChild(item, cookie);
   }
     
   itemdata = (wxbTreeItemData*)tree->GetItemData(item);  
   itemdata->SetMarked(newstate);
   tree->SetItemImage(item, newstate, wxTreeItemIcon_Normal);
   tree->SetItemImage(item, newstate, wxTreeItemIcon_Selected);
   tree->Refresh();
   
   if (tree->GetSelection() == item) {
      for (long i = 0; i < list->GetItemCount(); i++) {
         list->SetItemImage(i, newstate, 0); /* TODO: Find what these ints are for */
         list->SetItemImage(i, newstate, 1);
      }
   }
   
   UpdateTreeItemState(tree->GetItemParent(item));
}

/* Update a tree item state, and its parents' state */
void wxbRestorePanel::UpdateTreeItemState(wxTreeItemId item) {  
   if (!item.IsOk()) {
      return;
   }
   
   int state = 0;
       
#if wxCHECK_VERSION(2, 6, 0)
   wxTreeItemIdValue cookie;
#else
   long cookie;
#endif
   wxTreeItemId currentChild = tree->GetFirstChild(item, cookie);

   bool onechildmarked = false;
   bool onechildunmarked = false;

   while (currentChild.IsOk()) {
      state = ((wxbTreeItemData*)tree->GetItemData(currentChild))->GetMarked();
      switch (state) {
      case 0:
         onechildunmarked = true;
         break;
      case 1:
         onechildmarked = true;
         break;
      case 2:
         onechildmarked = true;
         onechildunmarked = true;
         break;
      }
      
      if (onechildmarked && onechildunmarked) {
         break;
      }
      
      currentChild = tree->GetNextChild(item, cookie);
   }
   
   if (tree->GetSelection() == item) {
      for (long i = 0; i < list->GetItemCount(); i++) {
         state = ((wxbTreeItemData*)list->GetItemData(i))->GetMarked();
         
         switch (state) {
         case 0:
            onechildunmarked = true;
            break;
         case 1:
            onechildmarked = true;
            break;
         case 2:
            onechildmarked = true;
            onechildunmarked = true;
            break;
         }
         
         if (onechildmarked && onechildunmarked) {
            break;
         }
      }
   }
   
   state = 0;
   
   if (onechildmarked && onechildunmarked) {
      state = 2;
   }
   else if (onechildmarked) {
      state = 1;
   }
   else if (onechildunmarked) {
      state = 0;
   }
   else { // no child, don't change anything
      UpdateTreeItemState(tree->GetItemParent(item));
      return;
   }
   
   wxbTreeItemData* itemdata = (wxbTreeItemData*)tree->GetItemData(item);
      
   itemdata->SetMarked(state);
   tree->SetItemImage(item, state, wxTreeItemIcon_Normal);
   tree->SetItemImage(item, state, wxTreeItemIcon_Selected);
   
   UpdateTreeItemState(tree->GetItemParent(item));
}

/* Refresh the whole tree. */
void wxbRestorePanel::RefreshTree() {
   /* Save current selection */
   wxArrayString current;
   
   wxTreeItemId item = currentTreeItem;
   
   while ((item.IsOk()) && (item != tree->GetRootItem())) {
      current.Add(tree->GetItemText(item));
      item = tree->GetItemParent(item);
   }

   /* Update the tree */
   UpdateTreeItem(tree->GetRootItem(), false, true);

   /* Reselect the former selected item */   
   item = tree->GetRootItem();
   
   if (current.Count() == 0) {
      tree->SelectItem(item);
      return;
   }
   
   bool match;
   
   for (int i = current.Count()-1; i >= 0; i--) {
#if wxCHECK_VERSION(2, 6, 0)
      wxTreeItemIdValue cookie;
#else
      long cookie;
#endif
      wxTreeItemId currentChild = tree->GetFirstChild(item, cookie);
      
      match = false;
      
      while (currentChild.IsOk()) {
         if (((wxbTreeItemData*)tree->GetItemData(currentChild))->GetName() == current[i]) {
            item = currentChild;
            match = true;
            break;
         }
   
         currentChild = tree->GetNextChild(item, cookie);
      }
      
      if (!match) break;
   }
   
   UpdateTreeItem(item, true, false); /* Update the list */
   
   tree->SelectItem(item);
}

void wxbRestorePanel::RefreshList() {
   if (currentTreeItem.IsOk()) {
      UpdateTreeItem(currentTreeItem, true, false); /* Update the list */
   }
}

/* Update first config, adapting settings to the job name selected */
void wxbRestorePanel::UpdateFirstConfig() {
   configPanel->Enable(false);
   wxbDataTokenizer* dt = wxbUtils::WaitForEnd(wxString(wxT(".defaults job=")) + configPanel->GetRowString(_("Job Name")) + wxT("\n"), true, false);
   /* job=RestoreFiles
    * pool=Default
    * messages=Standard
    * client=***
    * storage=File
    * where=/tmp/bacula-restores
    * level=0
    * type=Restore
    * fileset=***
    */
   
   wxString name, str;
   unsigned int i;
   int j;
   wxString client;
   bool dolistjobs = false;
   
   for (i = 0; i < dt->GetCount(); i++) {
      str = (*dt)[i];
      if ((j = str.Find('=')) > -1) {
         name = str.Mid(0, j);
         if (name == wxT("pool")) {
            configPanel->SetRowString(_("Pool"), str.Mid(j+1));
         }
         else if (name == wxT("client")) {
            str = str.Mid(j+1);
            if ((str != configPanel->GetRowString(_("Client"))) ||
                  (configPanel->GetRowString(_("Before"))) == wxT("")) {
               configPanel->SetRowString(_("Client"), str);
               dolistjobs = true;
            }
         }
         else if (name == wxT("storage")) {
            configPanel->SetRowString(_("Storage"), str.Mid(j+1));
         }
         else if (name == wxT("fileset")) {
            str = str.Mid(j+1);
            if ((str != configPanel->GetRowString(_("Fileset"))) ||
                  (configPanel->GetRowString(_("Before"))) == wxT("")) {
               configPanel->SetRowString(_("Fileset"), str);
               dolistjobs = true;
            }
         }
      }
   }
      
   delete dt;
   
   if (dolistjobs) {
      //wxTheApp->Yield(false);
      CmdListJobs();
   }
   configPanel->Enable(true);
}

/* 
 * Update second config.
 * 
 * Run Restore job
 * JobName:    RestoreFiles
 * Bootstrap:  /var/lib/bacula/restore.bsr
 * Where:      /tmp/bacula-restores
 * Replace:    always
 * FileSet:    Full Set
 * Client:     tom-fd
 * Storage:    File
 * When:       2004-04-18 01:18:56
 * Priority:   10
 * OK to run? (yes/mod/no):
 * 
 */
bool wxbRestorePanel::UpdateSecondConfig(wxbDataTokenizer* dt) {
   unsigned int i;
   for (i = 0; i < dt->GetCount(); i++) {
      if ((*dt)[i].Find(_("Run Restore job")) == 0)
         break;
   }
   
   if ((i + 10) > dt->GetCount()) {
      return false;
   }
   
   int k;
   
   if ((k = (*dt)[++i].Find(_("JobName:"))) != 0) return false;
   restorePanel->SetRowString(_("Job Name"), (*dt)[i].Mid(10).Trim(false).RemoveLast());
   if ((k = (*dt)[++i].Find(_("Bootstrap:"))) != 0) return false;
   restorePanel->SetRowString(_("Bootstrap"), (*dt)[i].Mid(10).Trim(false).RemoveLast());
   if ((k = (*dt)[++i].Find(_("Where:"))) != 0) return false;
   restorePanel->SetRowString(_("Where"), (*dt)[i].Mid(10).Trim(false).RemoveLast());
   
   if ((k = (*dt)[++i].Find(_("Replace:"))) != 0) return false;
   wxString str = (*dt)[i].Mid(10).Trim(false).RemoveLast();
   if (str == _("always")) restorePanel->SetRowSelection(_("Replace"), 0);
   else if (str == _("ifnewer")) restorePanel->SetRowSelection(_("Replace"), 1);
   else if (str == _("ifolder")) restorePanel->SetRowSelection(_("Replace"), 2);
   else if (str == _("never")) restorePanel->SetRowSelection(_("Replace"), 3);
   else restorePanel->SetRowSelection(_("Replace"), 0);

   if ((k = (*dt)[++i].Find(_("FileSet:"))) != 0) return false;
   restorePanel->SetRowString(_("Fileset"), (*dt)[i].Mid(10).Trim(false).RemoveLast());
   if ((k = (*dt)[++i].Find(_("Client:"))) != 0) return false;
   restorePanel->SetRowString(_("Client"), (*dt)[i].Mid(10).Trim(false).RemoveLast());
   if ((k = (*dt)[++i].Find(_("Storage:"))) != 0) return false;
   restorePanel->SetRowString(_("Storage"), (*dt)[i].Mid(10).Trim(false).RemoveLast());
   if ((k = (*dt)[++i].Find(_("When:"))) != 0) return false;
   restorePanel->SetRowString(_("When"), (*dt)[i].Mid(10).Trim(false).RemoveLast());
   i++;        /* Skip catalog field */
   if ((k = (*dt)[++i].Find(_("Priority:"))) != 0) return false;
   restorePanel->SetRowString(_("Priority"), (*dt)[i].Mid(10).Trim(false).RemoveLast());
   cfgUpdated = 0;
   
   restorePanel->Layout();
   
   return true;
}

/*----------------------------------------------------------------------------
   Status function
  ----------------------------------------------------------------------------*/

/* Set current status by enabling/disabling components */
void wxbRestorePanel::SetStatus(status_enum newstatus) {
   switch (newstatus) {
   case disabled:
      centerSizer->Remove(configPanel);
      centerSizer->Remove(restorePanel);
      centerSizer->Remove(treelistPanel);
      treelistPanel->Show(false);
      restorePanel->Show(false);
      centerSizer->Add(configPanel, 1, wxEXPAND);
      configPanel->Show(true);
      configPanel->Layout();
      centerSizer->Layout();
      this->Layout();
      start->SetLabel(_("Enter restore mode"));
      start->Enable(false);
      configPanel->Enable(false);
      tree->Enable(false);
      list->Enable(false);
      gauge->Enable(false);
      cancel->Enable(false);
      cfgUpdated = 0;
      cancelled = 0;
      break;
   case finished:
      centerSizer->Remove(configPanel);
      centerSizer->Remove(restorePanel);
      centerSizer->Remove(treelistPanel);
      treelistPanel->Show(false);
      restorePanel->Show(false);
      centerSizer->Add(configPanel, 1, wxEXPAND);
      configPanel->Show(true);
      configPanel->Layout();
      centerSizer->Layout();
      this->Layout();
      tree->DeleteAllItems();
      list->DeleteAllItems();
      configPanel->ClearRowChoices(_("Client"));
      configPanel->ClearRowChoices(_("Before"));
      wxbMainFrame::GetInstance()->EnablePanels();
      newstatus = activable;
   case activable:
      cancelled = 0;
      start->SetLabel(_("Enter restore mode"));
      start->Enable(true);
      configPanel->Enable(false);
      tree->Enable(false);
      list->Enable(false);
      gauge->Enable(false);
      cancel->Enable(false);
      cfgUpdated = 0;
      break;
   case entered:
      wxbMainFrame::GetInstance()->DisablePanels(this);
      gauge->SetValue(0);
      start->Enable(false);
      //start->SetLabel(_("Choose files to restore"));
      configPanel->Enable(true);
      tree->Enable(false);
      list->Enable(false);
      cancel->Enable(true);
      cfgUpdated = 0;
      break;
   case listing:
      break;
   case choosing:
      start->Enable(true);
      start->SetLabel(_("Restore"));
      centerSizer->Remove(configPanel);
      configPanel->Show(false);
      centerSizer->Add(treelistPanel, 1, wxEXPAND);
      treelistPanel->Show(true);
      treelistPanel->Layout();
      centerSizer->Layout();
      this->Layout();
      tree->Enable(true);
      list->Enable(true);
      SetWorking(false);
      break;
   case configuring:
      start->Enable(false);
      configPanel->Enable(false);
      tree->Enable(false);
      list->Enable(false);
      centerSizer->Remove(treelistPanel);
      treelistPanel->Show(false);
      centerSizer->Add(restorePanel, 1, wxEXPAND);
      restorePanel->Show(true);
      restorePanel->Layout();
      centerSizer->Layout();
      this->Layout();
      restorePanel->EnableApply(false);
      cancel->Enable(true);
      break;
   case restoring:
      start->SetLabel(_("Restoring..."));
      gauge->Enable(true);
      gauge->SetValue(0);
      start->Enable(false);
      configPanel->Enable(false);
      tree->Enable(false);
      list->Enable(false);
      SetWorking(true);
      break;
   }
   status = newstatus;
}

/*----------------------------------------------------------------------------
   UI related
  ----------------------------------------------------------------------------*/

void wxbRestorePanel::SetWorking(bool working) {
   this->working = working;
   if (working) {
      SetCursor(*wxHOURGLASS_CURSOR);
//      SetEvtHandlerEnabled(false); //EVTQUEUE
   }
//   else if (!processing) { /* Empty event queue if we aren't already doing this */ //EVTQUEUE
   else {
//      processing = true; //EVTQUEUE
      SetCursor(*wxSTANDARD_CURSOR);
//      SetEvtHandlerEnabled(true); //EVTQUEUE
/*      wxNode *node = pendingEvents->First(); //EVTQUEUE
      while ( node ) {
         wxEvent *event = (wxEvent *)node->Data();
         delete node;
   
         wxEvtHandler::ProcessEvent(*event);
         delete event;
   
         node = pendingEvents->First();
      }
      processing = false;*/
   }
}

bool wxbRestorePanel::IsWorking() {
   return this->working;
}

void wxbRestorePanel::EnableConfig(bool enable) {
   restorePanel->Enable(enable);
}

/*----------------------------------------------------------------------------
   Event handling
  ----------------------------------------------------------------------------*/


//EVTQUEUE
/*
bool wxbRestorePanel::ProcessEvent(wxEvent& event) {
   if (IsWorking() || processing) {
      wxEvent *eventCopy = event.Clone();
      
      pendingEvents->Append(eventCopy);
      return TRUE;
   }
   else {
      return wxEvtHandler::ProcessEvent(event);
   }
}
*/

void wxbRestorePanel::OnCancel(wxCommandEvent& event) {
   cancel->Enable(false);
   SetCursor(*wxHOURGLASS_CURSOR);
   CmdCancel();
   SetCursor(*wxSTANDARD_CURSOR);
}

void wxbRestorePanel::OnStart(wxCommandEvent& event) {
   if (IsWorking()) {
      return;
   }
   SetWorking(true);
   CmdStart();
   SetWorking(false);
}

void wxbRestorePanel::OnTreeChanging(wxTreeEvent& event) {
   if (IsWorking()) {
      event.Veto();
   }
}

void wxbRestorePanel::OnTreeExpanding(wxTreeEvent& event) {
   if (IsWorking()) {
      event.Veto();
      return;
   }
   //working = true;
   //CmdList(event.GetItem());
   if (tree->GetSelection() != event.GetItem()) {
      tree->SelectItem(event.GetItem());
   }
   //working = false;
}

void wxbRestorePanel::OnTreeChanged(wxTreeEvent& event) {
   if (IsWorking()) {
      return;
   }
   if (currentTreeItem == event.GetItem()) {
      return;
   }
   treeadd->Enable(false);
   treeremove->Enable(false);
   treerefresh->Enable(false);
   markWhenCommandDone = false;
   SetWorking(true);
   currentTreeItem = event.GetItem();
   CmdList(event.GetItem());
   if (markWhenCommandDone) {
      CmdMark(event.GetItem(), NULL, 0);
      tree->Refresh();
   }
   SetWorking(false);
   if (event.GetItem().IsOk()) {
      int status = ((wxbTreeItemData*)tree->GetItemData(event.GetItem()))->GetMarked();
      treeadd->Enable(status != 1);
      treeremove->Enable(status != 0);
   }
   treerefresh->Enable(true);
}

void wxbRestorePanel::OnTreeMarked(wxbTreeMarkedEvent& event) {
   if (IsWorking()) {
      if (tree->GetSelection() == event.GetItem()) {
         markWhenCommandDone = !markWhenCommandDone;
      }
      return;
   }
   SetWorking(true);
   markWhenCommandDone = false;
   CmdMark(event.GetItem(), NULL, 0);
   if (markWhenCommandDone) {
      CmdMark(event.GetItem(), NULL, 0);
      tree->Refresh();
   }
   tree->Refresh();
   SetWorking(false);
   if (event.GetItem().IsOk()) {
      int status = ((wxbTreeItemData*)tree->GetItemData(event.GetItem()))->GetMarked();
      treeadd->Enable(status != 1);
      treeremove->Enable(status != 0);
   }
}

void wxbRestorePanel::OnTreeAdd(wxCommandEvent& event) {
   if (IsWorking()) {
      return;
   }
   
   if (currentTreeItem.IsOk()) {
      SetWorking(true);
      CmdMark(currentTreeItem, NULL, 0, 1);
      tree->Refresh();
      treeadd->Enable(0);
      treeremove->Enable(1);
      SetWorking(false);
   }
}

void wxbRestorePanel::OnTreeRemove(wxCommandEvent& event) {
   if (IsWorking()) {
      return;
   }
   
   if (currentTreeItem.IsOk()) {
      SetWorking(true);
      CmdMark(currentTreeItem, NULL, 0, 0);
      tree->Refresh();
      treeadd->Enable(1);
      treeremove->Enable(0);
      SetWorking(false);
   }
}

void wxbRestorePanel::OnTreeRefresh(wxCommandEvent& event) {
   if (IsWorking()) {
      return;
   }
   
   SetWorking(true);
   RefreshTree();
   SetWorking(false);
}

void wxbRestorePanel::OnListMarked(wxbListMarkedEvent& event) {
   if (IsWorking()) {
      return;
   }
   
   if (list->GetSelectedItemCount() == 0) {
      return;
   }
   
   SetWorking(true);  
   
   long* items = new long[list->GetSelectedItemCount()];
   
   int num = 0;
   
   long item = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   while (item != -1) {
      items[num] = item;
      num++;
      item = list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   }
   
   CmdMark(wxTreeItemId(), items, num);
   
   delete[] items;
   
   wxListEvent listevt;
   
   OnListChanged(listevt);
   
   event.Skip();
   tree->Refresh();
   SetWorking(false);
}

void wxbRestorePanel::OnListActivated(wxListEvent& event) {
   if (IsWorking()) {
      return;
   }
   SetWorking(true);
   long item = event.GetIndex();
//   long item = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
   if (item > -1) {
      wxbTreeItemData* itemdata = (wxbTreeItemData*)list->GetItemData(item);
      wxString name = itemdata->GetName();
      event.Skip();

      wxString itemStr;

#if wxCHECK_VERSION(2, 6, 0)
      wxTreeItemIdValue cookie;
#else
      long cookie;
#endif

      if (IsPathSeparator(name.GetChar(name.Length()-1))) {
         wxTreeItemId currentChild = tree->GetFirstChild(currentTreeItem, cookie);

         while (currentChild.IsOk()) {
            wxString name2 = ((wxbTreeItemData*)tree->GetItemData(currentChild))->GetName();
            if (name2 == name) {
               //tree->UnselectAll();
               SetWorking(false);
               tree->Expand(currentTreeItem);
               tree->SelectItem(currentChild);
               //tree->Refresh();
               return;
            }
            currentChild = tree->GetNextChild(currentTreeItem, cookie);
         }
      }
   }
   SetWorking(false);
}

void wxbRestorePanel::OnListChanged(wxListEvent& event) {
   if (IsWorking()) {
      return;
   }
 
   listadd->Enable(false);
   listremove->Enable(false);
   
   bool marked = false;
   bool unmarked = false;
   
   long item = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   while (item != -1) {
      switch (((wxbTreeItemData*)list->GetItemData(item))->GetMarked()) {
      case 0:
         unmarked = true;
         break;
      case 1:
         marked = true;
         break;
      case 2:
         marked = true;
         unmarked = true;
         break;
      default:
         break;
         // Should never happen
      }
      if (marked && unmarked) break;
      item = list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   }
   
   listadd->Enable(unmarked);
   listremove->Enable(marked);
}

void wxbRestorePanel::OnListAdd(wxCommandEvent& event) {
   if (IsWorking()) {
      return;
   }
   
   SetWorking(true);
   
   long* items = new long[list->GetSelectedItemCount()];
   
   int num = 0;
   
   long item = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   while (item != -1) {
      items[num] = item;
      num++;
      item = list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   }
     
   CmdMark(wxTreeItemId(), items, num, 1);
   
   delete[] items;
   
   tree->Refresh();
   SetWorking(false);
   
   listadd->Enable(false);
   listremove->Enable(true);
}

void wxbRestorePanel::OnListRemove(wxCommandEvent& event) {
   if (IsWorking()) {
      return;
   }
   
   SetWorking(true);
   
   long* items = new long[list->GetSelectedItemCount()];
   
   int num = 0;
   
   long item = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   while (item != -1) {
      items[num] = item;
      num++;
      item = list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
   }
     
   CmdMark(wxTreeItemId(), items, num, 0);
   
   delete[] items;
   
   tree->Refresh();
   SetWorking(false);
   
   listadd->Enable(true);
   listremove->Enable(false);
}

void wxbRestorePanel::OnListRefresh(wxCommandEvent& event) 
{
   if (IsWorking()) {
      return;
   }
   
   SetWorking(true);
   RefreshList();
   SetWorking(false);
}

void wxbRestorePanel::OnConfigUpdated(wxCommandEvent& event) 
{
   if (status == entered) {
      if (event.GetId() == ConfigJobName) {
         if (IsWorking()) {
            return;
         }
         SetWorking(true);
         UpdateFirstConfig();
         SetWorking(false);
      }
      else if ((event.GetId() == ConfigClient) || (event.GetId() == ConfigFileset)) {
         if (IsWorking()) {
            return;
         }
         SetWorking(true);
         configPanel->Enable(false);
         CmdListJobs();
         configPanel->Enable(true);
         SetWorking(false);
      }
      cfgUpdated = cfgUpdated | (1 << event.GetId());
   }
   else if (status == configuring) {
      restorePanel->EnableApply(true);
      cfgUpdated = cfgUpdated | (1 << event.GetId());
   }
}

void wxbRestorePanel::OnConfigOk(wxCommandEvent& WXUNUSED(event)) {
   if (status != configuring) return;
   if (IsWorking()) {
      return;
   }
   SetWorking(true);
   CmdStart();
   SetWorking(false);
}

void wxbRestorePanel::OnConfigApply(wxCommandEvent& WXUNUSED(event)) {
   if (status != configuring) return;
   if (IsWorking()) {
      return;
   }
   SetWorking(true);
   CmdConfigApply();
   if (cfgUpdated == 0) {
      restorePanel->EnableApply(false);
   }
   SetWorking(false);  
}

void wxbRestorePanel::OnConfigCancel(wxCommandEvent& WXUNUSED(event)) {
   if (status != configuring) return;
   if (IsWorking()) {
      return;
   }
   SetWorking(true);
   CmdConfigCancel();
   SetWorking(false);
}

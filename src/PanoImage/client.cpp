/////////////////////////////////////////////////////////////////////////////
// Name:        client.h
// Purpose:     Client for wxSocket demo
// Author:      Guillermo Rodriguez Garcia <guille@iies.es>
// Modified by:
// Created:     1999/09/19
// RCS-ID:      $Id$
// Copyright:   (c) 1999 Guillermo Rodriguez Garcia
// Licence:     wxWindows licence
//              adopted for hugin by Kai-Uwe Behrmann 2003
/////////////////////////////////////////////////////////////////////////////

#include "client.h"


// --------------------------------------------------------------------------
// event tables and other macros for wxWindows
// --------------------------------------------------------------------------

BEGIN_EVENT_TABLE(Client, wxWindow)
  EVT_SOCKET(SOCKET_ID,     Client::OnSocketEvent)
END_EVENT_TABLE()

// ==========================================================================
// implementation
// ==========================================================================

Client * m_client;

// --------------------------------------------------------------------------
// the application class
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// classes
// --------------------------------------------------------------------------

// constructor
Client::Client() 
{
  DEBUG_INFO ( "" )
    // start with Start(port)
  DEBUG_INFO ( "" )
}

Client::~Client()
{
  DEBUG_INFO ( "" )
  // No delayed deletion here, as the frame is dying anyway
/*  if ( m_sock->Ok() ) {
    DEBUG_INFO ( "" )
    m_sock->Destroy();
    delete m_sock;
  }*/
  DEBUG_INFO ( "" )
}

bool Client::Start(int port)
{
  DEBUG_INFO ( _("start") )
  m_busy = TRUE;
  DEBUG_INFO ( "" )
  // Create the socket
  m_sock = new wxSocketClient();

  // Setup the event handler and subscribe to most events
  DEBUG_INFO ( "" )
  m_sock->SetEventHandler(*this, SOCKET_ID);
  DEBUG_INFO ( "" )
  m_sock->SetNotify(wxSOCKET_CONNECTION_FLAG |
                    wxSOCKET_INPUT_FLAG |
                    wxSOCKET_LOST_FLAG);
  DEBUG_INFO ( "" )
  m_sock->Notify(TRUE);

  DEBUG_INFO ( "" )
  // direct connect
  wxIPV4address addr;

  // Ask user for server address
  wxString hostname = _("localhost");

  addr.Hostname(hostname);
  addr.Service(port);

  DEBUG_INFO ( "" )
  m_sock->Connect(addr, FALSE);
/*  DEBUG_INFO ( "" )
  m_sock->WaitOnConnect(1);


  DEBUG_INFO ( "" )
  if (m_sock->IsConnected()) {
  } else {
    m_sock->Close();
    wxMessageBox(_("Can't connect to the specified host"), _("Alert !"));
  }
*/  
  m_busy = FALSE;
//  UpdateStatusBar();
  DEBUG_INFO ( _("end") )

  return TRUE;
}

// event handlers
void Client::GetServerData(wxSocketBase *sock)
{ 
  DEBUG_INFO ( _("start") )
  unsigned char len;
  char *buf;

  // Receive data from socket and send it back. We will first
  // get a byte with the buffer size, so we can specify the
  // exact size and use the wxSOCKET_WAITALL flag. Also, we
  // disabled input events so we won't have unwanted reentrance.
  // This way we can avoid the infamous wxSOCKET_BLOCK flag.
  
  DEBUG_INFO ( "" )
  sock->SetFlags(wxSOCKET_WAITALL|wxSOCKET_BLOCK);
  
  // Read the size
  DEBUG_INFO ( "" )
  if ( sock->Ok() )  {   // needed to connect imideately
    sock->Read(&len, 1);
    DEBUG_INFO ( "read " << (int) len )
  } else {
    DEBUG_INFO ( "Closed" )
    return;
  }
  DEBUG_INFO ( "" )
  buf = new char[len];
  DEBUG_INFO ( "" )
  
  // Read the data
  sock->Read(buf, len);
  DEBUG_INFO ( "read " << buf )

  DEBUG_INFO ( buf )
  wxString s = buf;
  frame->SetStatusText(s << " loaded", 0);

  // Write it back
  sock->Write(buf, len);

  DEBUG_INFO ( "" )
  // load file
  frame->ShowFile (buf);

  DEBUG_INFO ( s << " " << buf )
  delete[] buf;
  
  frame->SetStatusText(_("ready"), 1);
  DEBUG_INFO ( _("end") )
} 

void Client::OnSocketEvent(wxSocketEvent& event)
{
  DEBUG_INFO ( _("begin") << "  busy " << m_busy )
  UpdateStatusBar();
  if ( m_busy ) {
    event.Skip();
    DEBUG_INFO ( _("skipping event!!") )
    return;
  }
  wxString s = ("now  ");
  wxSocketBase *sock = event.GetSocket();

  switch(event.GetSocketEvent())
  {
    case wxSOCKET_INPUT      : s.Append(_("wxSOCKET_INPUT\n")); break;
    case wxSOCKET_LOST       : s.Append(_("wxSOCKET_LOST\n")); break;
    case wxSOCKET_CONNECTION : s.Append(_("wxSOCKET_CONNECTION\n")); break;
    default                  : s.Append(_("Unexpected event !\n")); break;
  }

  // Now we process the event
  DEBUG_INFO ( s )
  switch(event.GetSocketEvent())
  {
    case wxSOCKET_CONNECTION: {
      DEBUG_INFO ( "" )
      unsigned char handshake (0xBF);  // send handshake
      sock->Write ( &handshake, 1 );
      shaking = TRUE;
      DEBUG_INFO ( "handshake to server sent" )
      break;
      }
    case wxSOCKET_INPUT:
    {
      DEBUG_INFO ( "" )
      // We disable input events, so that the test doesn't trigger
      // wxSocketEvent again.
      sock->SetNotify(wxSOCKET_LOST_FLAG);

      // Which test are we going to run?
      unsigned char c;
      sock->Read(&c, 1);
      DEBUG_INFO ( "read " << c )

      // Handshake section
      if ( shaking ) {    // We are in Handshake with server.
        if ( c == 0xBA ) {// Is the server answere ours.
          sock->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
          DEBUG_INFO ( "connection to server accepted" )
        } else {
          sock->Close();  // refusing
        }
        shaking = FALSE;  // We shake only once per handshake.
      }

      switch (c)
      {
        case 0xBE: GetServerData(sock); break;
        case 0xBA: s = "Connection ok"; break;      // let the handshake pass
//        case 0xCE: Test2(sock); break;
//        case 0xDE: Test3(sock); break;
        default : s = "Unexpected event !\nLeaving!";sock->Close();return;break;
      }
      DEBUG_INFO ( s )
      sock->Discard();    // cleanup 
      if ( sock->LastCount() > 0 )
        DEBUG_INFO ( "bytes " << sock->LastCount() )

      // Enable input events again.
      break;
    }
    case wxSOCKET_LOST:
    {
//      sock->Destroy();
      break;
    }
    default: ;
  }

//  sock->Write ( "0", 1 );
  DEBUG_INFO ( _("end") << " sent " << "0 " << sock->LastCount() )
}

// convenience functions

void Client::UpdateStatusBar()
{
  wxString s ("");

  if (!m_sock->IsConnected())
  {
    s.Printf(_("Not connected"));
  }
  else
  {
    wxIPV4address addr;
    m_sock->GetPeer(addr);
    s.Printf(_("%s : %d"), (addr.Hostname()).c_str(), addr.Service());
  }

  frame->SetStatusText(s, 1);
  DEBUG_INFO ( _("end") )
}


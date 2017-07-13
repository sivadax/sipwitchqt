/*
 * Copyright 2017 Tycho Softworks.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "context.hpp"

Event::Data::Data() :
expires(-1), hops(0), status(0), natted(false), context(nullptr), event(nullptr), authorization(nullptr), association(NONE)
{
}

Event::Data::Data(eXosip_event_t *evt, Context *ctx) :
expires(-1), hops(0), status(0), natted(false), context(ctx), event(evt), authorization(nullptr), association(NONE)
{
    // ignore constructor parser if empty event;
    if(!evt) {
        context = nullptr;
        return;
    }

    // parse contact records and longest expiration from exosip2 event
    if(evt->response) {
        status = evt->response->status_code;
        //parseContacts(evt->response->contacts);
    }
    else if(evt->request) {
        if(osip_message_get_authorization(evt->request, 0, &authorization) != 0 || !authorization->username || !authorization->response)
            authorization = nullptr;
        parseSource(evt->request->vias);
        parseContacts(evt->request->contacts);
    }

    // parse out authorization for later use
    if(authorization && authorization->username)
        userid = Util::removeQuotes(authorization->username);
    if(authorization && authorization->response)
        digest = Util::removeQuotes(authorization->response);
    if(authorization && authorization->nonce)
        nonce = Util::removeQuotes(authorization->nonce);
    if(authorization && authorization->realm)
        realm = Util::removeQuotes(authorization->realm);
    if(authorization && authorization->algorithm)
        algorithm = Util::removeQuotes(authorization->algorithm);
}

Event::Data::~Data()
{
    if(event) {
        eXosip_event_free(event);
        event = nullptr;
    }
}

// find effective source address of requesting endpoint, or address of nat
void Event::Data::parseSource(const osip_list_t& list)
{
    int pos = 0;
    osip_via_t *via;
    Address nat;

    while(osip_list_eol(&list, pos) == 0) {
        via = (osip_via_t *)osip_list_get(&list, pos++);
        ++hops;
        if(via->host) {
            const char *addr = via->host;
            quint16 port = 5060, rport = 0;
            if(via->port)
                port = atoi(via->port);
            osip_uri_param_t *param = nullptr;
            osip_via_param_get_byname(via, (char *)"rport", &param);
            if(param && param->gvalue)
                rport = atoi(param->gvalue);
            if(via->port)
                port = atoi(via->port);
            source = Address(addr, port);

            // top nat only
            if(!nat && rport) {
                param = nullptr;
                osip_via_param_get_byname(via, (char *)"received", &param);
                if(param && param->gvalue)
                    nat = Address(param->gvalue, rport);
            }
        }
    }

    // nat overrides last via contact source address
    if(nat) {
        natted = true;
        source = nat;
    }

}

void Event::Data::parseContacts(const osip_list_t& list)
{
    int pos = 0;
    while(osip_list_eol(&list, pos) == 0) {
        auto contact = (osip_contact_t *)osip_list_get(&list, pos++);
        if(contact && contact->url) {
            osip_uri_t *uri = contact->url;
            osip_uri_param_t *param = nullptr;
            osip_contact_param_get_byname(contact, (char *)"expires", &param);
            int duration = -1;
            if(param && param->gvalue) {
                auto duration = osip_atoi(param->gvalue);
                if(duration > expires)
                    expires = duration;
            }
            if(natted)  // really make sure contact is natted too...
                contacts << Contact(source, duration, uri->username);
            else {
                quint16 port = 5060;
                if(uri->port && uri->port[0])
                    port = atoi(uri->port);
                contacts << Contact(uri->host, port, duration, uri->username);
            }
        }
    }
}

Event::Event()
{
    d = new Event::Data();
}

Event::Event(eXosip_event_t *evt, Context *ctx)
{
    d = new Event::Data(evt, ctx);
}

Event::Event(const Event& copy) :
d(copy.d)
{
}

Event::~Event()
{
}

// used for events that support only one contact object...
const Contact Event::contact() const
{
    if(d->contacts.size() != 1)
        return Contact();
    return d->contacts[0];
}

const QString Event::protocol() const
{
    Q_ASSERT(d->context != nullptr);
    return d->context->type();
}

QDebug operator<<(QDebug dbg, const Event& ev)
{
    if(ev)
        dbg.nospace() << "Event(" << ev.toString() << ",cid=" << ev.cid() << ",did=" << ev.did() << ",proto=" << ev.protocol() << ")";
    else
        dbg.nospace() << "Event(timeout)";
    return dbg.maybeSpace();
}

const QString Event::toString() const
{
    if(!d->event)
        return "none";

    QString result = QString::number(d->event->type);
    switch(d->event->type) {
    case EXOSIP_REGISTRATION_SUCCESS:
        return result + "/register";
    case EXOSIP_CALL_INVITE:
        return result + "/invite";
    case EXOSIP_CALL_REINVITE:
        return result + "/reinvite";
    case EXOSIP_CALL_NOANSWER:
    case EXOSIP_SUBSCRIPTION_NOANSWER:
    case EXOSIP_NOTIFICATION_NOANSWER:
        return result + "/noanswer";
    case EXOSIP_MESSAGE_PROCEEDING:
    case EXOSIP_NOTIFICATION_PROCEEDING:
    case EXOSIP_CALL_MESSAGE_PROCEEDING:
    case EXOSIP_SUBSCRIPTION_PROCEEDING:
    case EXOSIP_CALL_PROCEEDING:
        return result + "/proceed";
    case EXOSIP_CALL_RINGING:
        return result + "/ring";
    case EXOSIP_MESSAGE_ANSWERED:
    case EXOSIP_CALL_ANSWERED:
    case EXOSIP_CALL_MESSAGE_ANSWERED:
    case EXOSIP_SUBSCRIPTION_ANSWERED:
    case EXOSIP_NOTIFICATION_ANSWERED:
        return result + "/answer";
    case EXOSIP_SUBSCRIPTION_REDIRECTED:
    case EXOSIP_NOTIFICATION_REDIRECTED:
    case EXOSIP_CALL_MESSAGE_REDIRECTED:
    case EXOSIP_CALL_REDIRECTED:
    case EXOSIP_MESSAGE_REDIRECTED:
        return result + "/redirect";
    case EXOSIP_REGISTRATION_FAILURE:
        return result + "/noreg";
    case EXOSIP_SUBSCRIPTION_REQUESTFAILURE:
    case EXOSIP_NOTIFICATION_REQUESTFAILURE:
    case EXOSIP_CALL_REQUESTFAILURE:
    case EXOSIP_CALL_MESSAGE_REQUESTFAILURE:
    case EXOSIP_MESSAGE_REQUESTFAILURE:
        return result + "/failed";
    case EXOSIP_SUBSCRIPTION_SERVERFAILURE:
    case EXOSIP_NOTIFICATION_SERVERFAILURE:
    case EXOSIP_CALL_SERVERFAILURE:
    case EXOSIP_CALL_MESSAGE_SERVERFAILURE:
    case EXOSIP_MESSAGE_SERVERFAILURE:
        return result + "/server";
    case EXOSIP_SUBSCRIPTION_GLOBALFAILURE:
    case EXOSIP_NOTIFICATION_GLOBALFAILURE:
    case EXOSIP_CALL_GLOBALFAILURE:
    case EXOSIP_CALL_MESSAGE_GLOBALFAILURE:
    case EXOSIP_MESSAGE_GLOBALFAILURE:
        return result + "/global";
    case EXOSIP_CALL_ACK:
        return result + "/ack";
    case EXOSIP_CALL_CLOSED:
    case EXOSIP_CALL_RELEASED:
        return result + "/bye";
    case EXOSIP_CALL_CANCELLED:
        return result + "/cancel";
    case EXOSIP_MESSAGE_NEW:
    case EXOSIP_CALL_MESSAGE_NEW:
    case EXOSIP_IN_SUBSCRIPTION_NEW:
        return result + "/new";
    case EXOSIP_SUBSCRIPTION_NOTIFY:
        return result + "/notify";
    default:
        break;
    }
    return "unknown";
}

namespace Util {
    QString removeQuotes(const QString& str)
    {
        if(str.startsWith("\"") && str.endsWith("\""))
            return str.mid(1, str.length() - 2);
        else if(str.startsWith("'") && str.endsWith("'"))
            return str.mid(1, str.length() - 2);
        return str;
    }
};


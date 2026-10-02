from datetime import datetime, timezone
import os
import uuid
from typing import Any, Dict, Optional


# Installed via requirements.txt in the Docker image
from requests import Session
from requests.auth import HTTPBasicAuth
from zeep import Client, Settings
from zeep.transports import Transport
from lxml import etree


class DomibusWSClient:

  def __init__(
      self,
      base_url: str,
      username: Optional[str] = None,
      password: Optional[str] = None,
  ):
    self.base_url = base_url.rstrip("/")
    self.wsdl_url = f"{self.base_url}/services/wsplugin?wsdl"

    session = Session()
    if username and password:
      session.auth = HTTPBasicAuth(username, password)

    settings = Settings(strict=False, xml_huge_tree=True)
    transport = Transport(session=session, timeout=120)

    print(f"Loading WSDL from: {self.wsdl_url} ...")
    self.client = Client(
        wsdl=self.wsdl_url, transport=transport, settings=settings
    )

  def send_message(
        self,
        from_party_id: str,
        to_party_id: str,
        payload_meta: Dict[str, Any],
        service: str = "bdx:noprocess",
        service_type: str = "tc1",
        action: str = "TC1Leg1",
        conversation_id: Optional[str] = None,
    ) -> Any:
        conv_id = conversation_id or str(uuid.uuid4())
        msg_id = f"{uuid.uuid4()}@domibus.eu"
        payload_cid = "cid:message"
        timestamp = datetime.now(timezone.utc)

        # 1. Payload data
        if payload_meta.get("is_file_on_disk"):
            with open(payload_meta["file_path"], "rb") as f:
                payload_content = f.read()
        else:
            payload_content = payload_meta["data"]

        # 2. LargePayloadType for the submitRequest body
        payload_entry = {
            "value": payload_content,
            "payloadId": payload_cid,
            "contentType": payload_meta["content_type"],
        }

        # 3. Create PartyId objects using the schema type from the WSDL
        PartyIdType = self.client.get_type(
            "{http://docs.oasis-open.org/ebxml-msg/ebms/v3.0/ns/core/200704/}PartyId"
        )
        party_type_uri = "urn:oasis:names:tc:ebcore:partyid-type:unregistered"

        from_party = PartyIdType(_value_1=from_party_id, type=party_type_uri)
        to_party = PartyIdType(_value_1=to_party_id, type=party_type_uri)

        # 4. ebMS3 Header
        ebms_header = {
            "UserMessage": {
                "MessageInfo": {
                    "Timestamp": timestamp,
                    "MessageId": msg_id,
                },
                "PartyInfo": {
                    "From": {
                        "PartyId": from_party,
                        "Role": "http://docs.oasis-open.org/ebxml-msg/ebms/v3.0/ns/core/200704/sender",
                    },
                    "To": {
                        "PartyId": to_party,
                        "Role": "http://docs.oasis-open.org/ebxml-msg/ebms/v3.0/ns/core/200704/receiver",
                    },
                },
                "CollaborationInfo": {
                    "Service": {"_value_1": service, "type": service_type},
                    "Action": action,
                    "ConversationId": conv_id,
                },
                # Required by the "eDeliveryPropertySet" property set in the PMode
                "MessageProperties": {
                    "Property": [
                        {"_value_1": f"{party_type_uri}:{from_party_id}", "name": "originalSender"},
                        {"_value_1": f"{party_type_uri}:{to_party_id}", "name": "finalRecipient"},
                    ]
                },
                "PayloadInfo": {
                    "PartInfo": [
                        {
                            "href": payload_cid,
                            "PartProperties": {
                                "Property": [
                                    {"_value_1": payload_meta["content_type"], "name": "MimeType"},
                                    {"_value_1": payload_meta["filename"], "name": "FileName"},
                                ]
                            },
                        }
                    ]
                },
            }
        }

        messaging_element = self.client.get_element(
            "{http://docs.oasis-open.org/ebxml-msg/ebms/v3.0/ns/core/200704/}Messaging"
        )
        soap_headers = [messaging_element(**ebms_header)]

        # Debugging: set DOMIBUS_DEBUG=1 to print the SOAP header that is sent
        if os.environ.get("DOMIBUS_DEBUG"):
            from lxml import etree
            node = self.client.create_message(
                self.client.service, "submitMessage",
                payload=[payload_entry], _soapheaders=soap_headers,
            )
            header = node.find("{http://www.w3.org/2003/05/soap-envelope}Header")
            print(etree.tostring(header if header is not None else node, pretty_print=True).decode())

        response = self.client.service.submitMessage(
            payload=[payload_entry],
            _soapheaders=soap_headers,
        )

        return response
// Copyright (C) 2009-2023, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Util/Xml.h>

namespace anki {

Error XmlElement::check() const
{
	Error err = Error::kNone;
	if(m_el == nullptr)
	{
		ANKI_UTIL_LOGE("Empty element");
		err = Error::kUserData;
	}

	return err;
}

Error XmlElement::getText(CString& out) const
{
	ANKI_CHECK(check());
	out = (m_el->GetText()) ? CString(m_el->GetText()) : CString();
	return Error::kNone;
}

Error XmlElement::getChildElementOptional(CString name, XmlElement& out) const
{
	const Error err = check();
	if(!err)
	{
		out = XmlElement(m_el->FirstChildElement(&name[0]), m_pool);
	}
	else
	{
		out = XmlElement();
	}

	return err;
}

Error XmlElement::getChildElement(CString name, XmlElement& out) const
{
	Error err = check();
	if(err)
	{
		out = XmlElement();
		return err;
	}

	err = getChildElementOptional(name, out);
	if(err)
	{
		return err;
	}

	if(!out)
	{
		ANKI_UTIL_LOGE("Cannot find tag: %s", &name[0]);
		err = Error::kUserData;
	}

	return err;
}

Error XmlElement::getNextSiblingElement(CString name, XmlElement& out) const
{
	const Error err = check();
	if(!err)
	{
		out = XmlElement(m_el->NextSiblingElement(&name[0]), m_pool);
	}
	else
	{
		out = XmlElement();
	}

	return err;
}

Error XmlElement::getSiblingElementsCount(U32& out) const
{
	ANKI_CHECK(check());
	const tinyxml2::XMLElement* el = m_el;

	out = 0;
	do
	{
		el = el->NextSiblingElement(m_el->Name());
		++out;
	} while(el);

	out -= 1;

	return Error::kNone;
}

Error XmlElement::getAttributeTextOptional(CString name, CString& out, Bool& attribPresent) const
{
	ANKI_CHECK(check());

	const tinyxml2::XMLAttribute* attrib = m_el->FindAttribute(&name[0]);
	if(!attrib)
	{
		attribPresent = false;
		return Error::kNone;
	}

	attribPresent = true;

	const char* value = attrib->Value();
	if(value)
	{
		out = value;
	}
	else
	{
		out = CString();
	}

	return Error::kNone;
}

} // end namespace anki

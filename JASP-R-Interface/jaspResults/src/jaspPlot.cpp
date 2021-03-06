#include "jaspPlot.h"

jaspPlot::~jaspPlot()
{
#ifdef JASP_RESULTS_DEBUG_TRACES
	JASPprint("Destructor of JASPplot("+title+") is called! ");
#endif

	finalizedHandler();
}

std::string jaspPlot::dataToString(std::string prefix)
{
	std::stringstream out;

	out <<
		prefix << "aspectRatio: "	<< _aspectRatio << "\n" <<
		prefix << "dims:        "	<< _width << "X" << _height << "\n" <<
		prefix << "error:       '"	<< _error << "': '" << _errorMessage << "'\n" <<
		prefix << "filePath:    "	<< _filePathPng << "\n" <<
		prefix << "status:      "	<< _status << "\n" <<
		prefix << "has plot:    "	<< (_plotObjSerialized.size() > 0 ? "yes" : "no") << "\n";

	if(_footnotes.size() > 0)
	{
		out << prefix << "footnotes:   \n";
		for(int i=0; i<_footnotes.size(); i++)
			out << prefix << "\t[" << i << "]:\t(" << _footnotes[i]["symbol"] << ") " << _footnotes[i]["text"] << "\n";
	}

	return out.str();
}

Json::Value jaspPlot::dataEntry()
{
	Json::Value data(jaspObject::dataEntry());

	data["title"]		= _title;
	data["convertible"]	= true;
	data["data"]		= _filePathPng;
	data["height"]		= _height;
	data["width"]		= _width;
	data["aspectRatio"]	= _aspectRatio;
	data["status"]		= _error == "" ? _status : "error";
	if(_error != "")
    {
		data["error"]					= Json::objectValue;
		data["error"]["type"]			= _error;
		data["error"]["errorMessage"]	= _errorMessage;
    }
	data["name"]		= getUniqueNestedName();
	data["footnotes"]	= _footnotes;

	return data;
}

void jaspPlot::addFootnote(std::string message, std::string symbol)
{
	Json::Value footnote(Json::objectValue);

	footnote["text"]	= message;
	footnote["symbol"]	= symbol;
	footnote["cols"]	= Json::nullValue;
	footnote["rows"]	= Json::nullValue;

	_footnotes.append(footnote);
}


void jaspPlot::setPlotObject(Rcpp::RObject obj)
{
	_filePathPng = "";

	if(!obj.isNULL())
	{
		Rcpp::Function tryToWriteImage("tryToWriteImageJaspResults");
		Rcpp::List writeResult = tryToWriteImage(Rcpp::_["width"] = _width, Rcpp::_["height"] = _height, Rcpp::_["plot"] = obj);

		if(writeResult.containsElementNamed("png"))
			_filePathPng = Rcpp::as<std::string>(writeResult[writeResult.findName("png")]);

		if(writeResult.containsElementNamed("error"))
		{
			_error			= "Error during writeImage";
			_errorMessage	= Rcpp::as<std::string>(writeResult[writeResult.findName("error")]);
		}
	}


	Rcpp::Function serialize("serialize");
	_plotObjSerialized = serialize(Rcpp::_["object"] = obj, Rcpp::_["connection"] = R_NilValue, Rcpp::_["ascii"] = true);
}

Rcpp::RObject jaspPlot::getPlotObject()
{
	if(_plotObjSerialized.size() == 0)
		return NULL;

	Rcpp::Function unserialize("unserialize");
	return unserialize(_plotObjSerialized);
}

Json::Value jaspPlot::convertToJSON()
{
	Json::Value obj		= jaspObject::convertToJSON();

	obj["aspectRatio"]			= _aspectRatio;
	obj["width"]				= _width;
	obj["height"]				= _height;
	obj["error"]				= _error;
	obj["status"]				= _status;
	obj["errorMessage"]			= _errorMessage;
	obj["filePathPng"]			= _filePathPng;
	obj["footnotes"]			= _footnotes;
	obj["plotObjSerialized"]	= std::string(_plotObjSerialized.begin(), _plotObjSerialized.end());


	return obj;
}

void jaspPlot::convertFromJSON_SetFields(Json::Value in)
{
	jaspObject::convertFromJSON_SetFields(in);

	_aspectRatio	= in.get("aspectRatio",		0.0f).asDouble();
	_width			= in.get("width",			-1).asInt();
	_height			= in.get("height",			-1).asInt();
	_error			= in.get("error",			"null").asString();
	_status			= in.get("status",			"complete").asString();
	_errorMessage	= in.get("errorMessage",	"null").asString();
	_filePathPng	= in.get("filePathPng",		"null").asString();
	_footnotes		= in.get("footnotes",		Json::arrayValue);

	std::string jsonPlotObjStr = in.get("plotObjSerialized", "").asString();
	_plotObjSerialized = Rcpp::Vector<RAWSXP>(jsonPlotObjStr.begin(), jsonPlotObjStr.end());
}

std::string jaspPlot::toHtml()
{
	std::stringstream out;

	out << "<div class=\"status " << _status << "\">" "\n"
		<< htmlTitle() << "\n";

	if(_error != "" || _errorMessage != "")
	{
		out << "<p class=\"error\">\n";
		if(_error		 != "") out << "error: <i>'" << _error << "'</i>";
		if(_errorMessage != "") out << (_error != "" ? " msg: <i>'" : "errormessage: <i>'") << _errorMessage << "'</i>";
		out << "\n</p>";
	}
	else
		out << "<img src=\"" << _filePathPng << "\" height=\"" << _height << "\" width=\"" << _width << "\" alt=\"a plot called " << _title << "\">";


	if(_footnotes.size() > 0)
	{
		out << "<h4>footnotes</h4>" "\n" "<ul>";

		for(Json::Value::UInt i=0; i<_footnotes.size(); i++)
		{
			std::string sym = _footnotes[i]["symbol"].asString() ;
			out << "<li>" << (sym == "" ? "" : "<i>(" + sym  + ")</i> " ) << _footnotes[i]["text"].asString() << "</li>" "\n";
		}

		out << "</ul>\n";
	}

	out << "</div>\n";

	return out.str();
}

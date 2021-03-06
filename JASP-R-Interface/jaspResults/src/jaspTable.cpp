#include "jaspTable.h"

std::string jaspColRowCombination::toString()
{
	bool ColumnsNotRows = colNames.size() + colOvertitles.size() > 0;

	std::stringstream out;
	out << "{ " << (ColumnsNotRows ? "col-" : "row-") << "combination with title(" << title << "), name("<<name<<") and " << (ColumnsNotRows ? "cols" : "rows") << ": [";
	
	out << "[" << (ColumnsNotRows ? colNames.toStyledString() : rowNames.toStyledString());
	out << "], does " << (overwrite? "" : "not ") << "overwrite and does " << (removeSeparator? "" : "not ") << "remove separator.";
	return out.str();
}

size_t jaspTable::lengthFromRObject(Rcpp::RObject rObj)
{
	if(rObj.isNULL())								return 0;
	else if(Rcpp::is<Rcpp::List>(rObj))				return lengthFromList((Rcpp::List)						rObj);
	else if(Rcpp::is<Rcpp::NumericVector>(rObj))	return lengthFromVector<REALSXP>((Rcpp::NumericVector)	rObj);
	else if(Rcpp::is<Rcpp::LogicalVector>(rObj))	return lengthFromVector<LGLSXP>((Rcpp::LogicalVector)	rObj);
	else if(Rcpp::is<Rcpp::IntegerVector>(rObj))	return lengthFromVector<INTSXP>((Rcpp::IntegerVector)	rObj);
	else if(Rcpp::is<Rcpp::StringVector>(rObj))		return lengthFromVector<STRSXP>((Rcpp::StringVector)	rObj);
	else if(Rcpp::is<Rcpp::CharacterVector>(rObj))	return lengthFromVector<STRSXP>((Rcpp::CharacterVector)	rObj);
	else Rf_error("Unexpected type..");

	return 0;

}


void jaspTable::setData(Rcpp::RObject newData)
{
	if(newData.isNULL())
	{
		_data.clear();
		return;
	}

	//Maybe this is overkill?
	if(Rcpp::is<Rcpp::DataFrame>(newData))				setDataFromList(convertFactorsToCharacters((Rcpp::DataFrame)	newData));
	else if(Rcpp::is<Rcpp::List>(newData))				setDataFromList((Rcpp::List)									newData);

	else if(Rcpp::is<Rcpp::NumericMatrix>(newData))		setDataFromMatrix<REALSXP>((Rcpp::NumericMatrix)	newData);
	else if(Rcpp::is<Rcpp::LogicalMatrix>(newData))		setDataFromMatrix<LGLSXP>((Rcpp::LogicalMatrix)		newData);
	else if(Rcpp::is<Rcpp::IntegerMatrix>(newData))		setDataFromMatrix<INTSXP>((Rcpp::IntegerMatrix)		newData);
	else if(Rcpp::is<Rcpp::StringMatrix>(newData))		setDataFromMatrix<STRSXP>((Rcpp::StringMatrix)		newData);
	else if(Rcpp::is<Rcpp::CharacterMatrix>(newData))	setDataFromMatrix<STRSXP>((Rcpp::CharacterMatrix)	newData);

	else if(Rcpp::is<Rcpp::NumericVector>(newData))		setDataFromVector<REALSXP>((Rcpp::NumericVector)	newData);
	else if(Rcpp::is<Rcpp::LogicalVector>(newData))		setDataFromVector<LGLSXP>((Rcpp::LogicalVector)		newData);
	else if(Rcpp::is<Rcpp::IntegerVector>(newData))		setDataFromVector<INTSXP>((Rcpp::IntegerVector)		newData);
	else if(Rcpp::is<Rcpp::StringVector>(newData))		setDataFromVector<STRSXP>((Rcpp::StringVector)		newData);
	else if(Rcpp::is<Rcpp::CharacterVector>(newData))	setDataFromVector<STRSXP>((Rcpp::CharacterVector)	newData);

	else
		Rf_error("Cannot set this kind of data to a jaspTable, it is not understood. Try a list, dataframe, vector or matrix instead.");

	notifyParentOfChanges();
}


void jaspTable::addOrSetColumnInData(std::vector<Json::Value> column, std::string colName)
{
	if(colName == "")
		_data.push_back(column);
	else
	{
		//find the right place to put it based on the name and do so
		int desiredColumnIndex = getDesiredColumnIndexFromNameForColumnAdding(colName);

		if(_data.size() <= desiredColumnIndex)
			_data.resize(desiredColumnIndex + 1); //colNames does this automagically

		_data[desiredColumnIndex]		= column;
		_colNames[desiredColumnIndex]	= colName; //Might overwrite an existing colName
	}
}

int jaspTable::getDesiredColumnIndexFromNameForColumnAdding(std::string colName)
{
	//First we check what the last filled colName is in case we need to add a new column at the end
	int lastFilledColName=-1, lastFilledColumn=-1;

	for(int i=0; i<_colNames.rowCount(); i++)
		if(_colNames[i] != "")
			lastFilledColName = i;

	for(int i=0; i<_data.size(); i++)
		if(_data[i].size() > 0)
			lastFilledColumn = i;

	//we also take max because we also want to make sure it is after the last
	int desiredIndex = std::max(lastFilledColumn, lastFilledColName) + 1;

	//And then check if actually we already had this columnName specified because in that case we can just put all the data there!
	for(int possibleColIndex=0; possibleColIndex<_colNames.rowCount(); possibleColIndex++)
		if(_colNames[possibleColIndex] == colName)
			desiredIndex = possibleColIndex;

	return desiredIndex;
}

int jaspTable::pushbackToColumnInData(std::vector<Json::Value> column, std::string colName, int equalizedColumnsLength, int previouslyAddedUnnamed)
{
	int desiredColumnIndex = getDesiredColumnIndexFromNameForRowAdding(colName, previouslyAddedUnnamed);

	if(desiredColumnIndex >= _colNames.rowCount() || _colNames[desiredColumnIndex] == "")
		previouslyAddedUnnamed++;

	if(_data.size() <= desiredColumnIndex)
		_data.resize(desiredColumnIndex + 1); //colNames does this automagically


	if(_data[desiredColumnIndex].size() < equalizedColumnsLength)
		_data[desiredColumnIndex].resize(equalizedColumnsLength);

	for(auto element : column)
		_data[desiredColumnIndex].push_back(element);

	if(colName != "")
		_colNames[desiredColumnIndex] = colName;

	return previouslyAddedUnnamed;
}

int jaspTable::getDesiredColumnIndexFromNameForRowAdding(std::string colName, int previouslyAddedUnnamed)
{
	int desiredIndex = -1;

	if(colName != "")
	{
		for(int possibleColIndex=0; possibleColIndex<_colNames.rowCount(); possibleColIndex++)
			if(_colNames[possibleColIndex] == colName)
				desiredIndex = possibleColIndex;

		if(desiredIndex != -1)
			return desiredIndex;
	}

	int foundUnnamed = 0;
	for(int col=0; col<_colNames.rowCount() || col < _data.size(); col++)
		if(_colNames[col] == "")
		{

			if(previouslyAddedUnnamed == foundUnnamed)
				return col;

			foundUnnamed++;
		}

	return _colNames.rowCount();
}

void jaspTable::setColumn(std::string columnName, Rcpp::RObject column)
{
	int colIndex = getDesiredColumnIndexFromNameForColumnAdding(columnName);

	if(Rcpp::is<Rcpp::NumericVector>(column))			setColumnFromVector<REALSXP>((Rcpp::NumericVector)	column, colIndex);
	else if(Rcpp::is<Rcpp::LogicalVector>(column))		setColumnFromVector<LGLSXP>((Rcpp::LogicalVector)	column, colIndex);
	else if(Rcpp::is<Rcpp::IntegerVector>(column))		setColumnFromVector<INTSXP>((Rcpp::IntegerVector)	column, colIndex);
	else if(Rcpp::is<Rcpp::StringVector>(column))		setColumnFromVector<STRSXP>((Rcpp::StringVector)	column, colIndex);
	else if(Rcpp::is<Rcpp::CharacterVector>(column))	setColumnFromVector<STRSXP>((Rcpp::CharacterVector)	column, colIndex);
	else if(Rcpp::is<Rcpp::List>(column))				setColumnFromList((Rcpp::List)						column,	colIndex);
	else Rf_error("Did not get a vector or list as column..");

	notifyParentOfChanges();
}

void jaspTable::addColumns(Rcpp::RObject newData)
{
	if(newData.isNULL())
		return;

	//Maybe this is overkill?
	if(Rcpp::is<Rcpp::DataFrame>(newData))				addColumnsFromList(convertFactorsToCharacters((Rcpp::DataFrame)	newData));
	else if(Rcpp::is<Rcpp::List>(newData))				addColumnsFromList((Rcpp::List)									newData);

	else if(Rcpp::is<Rcpp::NumericMatrix>(newData))		addColumnsFromMatrix<REALSXP>((Rcpp::NumericMatrix)	newData);
	else if(Rcpp::is<Rcpp::LogicalMatrix>(newData))		addColumnsFromMatrix<LGLSXP>((Rcpp::LogicalMatrix)	newData);
	else if(Rcpp::is<Rcpp::IntegerMatrix>(newData))		addColumnsFromMatrix<INTSXP>((Rcpp::IntegerMatrix)	newData);
	else if(Rcpp::is<Rcpp::StringMatrix>(newData))		addColumnsFromMatrix<STRSXP>((Rcpp::StringMatrix)	newData);
	else if(Rcpp::is<Rcpp::CharacterMatrix>(newData))	addColumnsFromMatrix<STRSXP>((Rcpp::CharacterMatrix)newData);

	else if(Rcpp::is<Rcpp::NumericVector>(newData))		addColumnFromVector<REALSXP>((Rcpp::NumericVector)	newData);
	else if(Rcpp::is<Rcpp::LogicalVector>(newData))		addColumnFromVector<LGLSXP>((Rcpp::LogicalVector)	newData);
	else if(Rcpp::is<Rcpp::IntegerVector>(newData))		addColumnFromVector<INTSXP>((Rcpp::IntegerVector)	newData);
	else if(Rcpp::is<Rcpp::StringVector>(newData))		addColumnFromVector<STRSXP>((Rcpp::StringVector)	newData);
	else if(Rcpp::is<Rcpp::CharacterVector>(newData))	addColumnFromVector<STRSXP>((Rcpp::CharacterVector)	newData);

	else
		Rf_error("Cannot add this kind of data as a column to a jaspTable, it is not understood. Try a list, dataframe, vector or matrix instead.");

	notifyParentOfChanges();
}

void jaspTable::addRows(Rcpp::RObject newData, Rcpp::CharacterVector rowNames)
{
	if(newData.isNULL())
		return;

	//Maybe this is overkill?
	if(Rcpp::is<Rcpp::DataFrame>(newData))				addRowsFromDataFrame((Rcpp::DataFrame)				newData);
	else if(Rcpp::is<Rcpp::List>(newData))				addRowsFromList((Rcpp::List)						newData, rowNames);

	else if(Rcpp::is<Rcpp::NumericMatrix>(newData))		addRowsFromMatrix<REALSXP>((Rcpp::NumericMatrix)	newData, rowNames);
	else if(Rcpp::is<Rcpp::LogicalMatrix>(newData))		addRowsFromMatrix<LGLSXP>((Rcpp::LogicalMatrix)		newData, rowNames);
	else if(Rcpp::is<Rcpp::IntegerMatrix>(newData))		addRowsFromMatrix<INTSXP>((Rcpp::IntegerMatrix)		newData, rowNames);
	else if(Rcpp::is<Rcpp::StringMatrix>(newData))		addRowsFromMatrix<STRSXP>((Rcpp::StringMatrix)		newData, rowNames);
	else if(Rcpp::is<Rcpp::CharacterMatrix>(newData))	addRowsFromMatrix<STRSXP>((Rcpp::CharacterMatrix)	newData, rowNames);

	else if(Rcpp::is<Rcpp::NumericVector>(newData))		addRowFromVector<REALSXP>((Rcpp::NumericVector)		newData, rowNames);
	else if(Rcpp::is<Rcpp::LogicalVector>(newData))		addRowFromVector<LGLSXP>((Rcpp::LogicalVector)		newData, rowNames);
	else if(Rcpp::is<Rcpp::IntegerVector>(newData))		addRowFromVector<INTSXP>((Rcpp::IntegerVector)		newData, rowNames);
	else if(Rcpp::is<Rcpp::StringVector>(newData))		addRowFromVector<STRSXP>((Rcpp::StringVector)		newData, rowNames);
	else if(Rcpp::is<Rcpp::CharacterVector>(newData))	addRowFromVector<STRSXP>((Rcpp::CharacterVector)	newData, rowNames);

	else
		Rf_error("Cannot add this kind of data as a row to a jaspTable, it is not understood. Try a list, dataframe, vector or matrix instead.");

	notifyParentOfChanges();
}

void jaspTable::addColumnsFromList(Rcpp::List newData)
{
	size_t elementLenghts = 0;
	for(int el=0; el<newData.size(); el++)
		elementLenghts = std::max(lengthFromRObject((Rcpp::RObject)newData[el]), elementLenghts);

	if(elementLenghts <= 1 && newData.size() > 1) //each entry is 1 or 0, this must be a single row with columnnames and not a set of rows with rownames..
	{
		Rcpp::List newColList;
		auto shield = new Rcpp::Shield<Rcpp::List>(newColList);
		newColList.push_back(newData);
		addColumnsFromList(newColList);
		delete shield;

		return;
	}

	std::vector<std::string> localColNames = extractElementOrColumnNames(newData);
	extractRowNames(newData, true);

	for(int col=0; col<newData.size(); col++)
		addOrSetColumnInData(jaspJson::RcppVector_to_VectorJson((Rcpp::RObject)newData[col]), localColNames.size() > col ? localColNames[col] : "");
}

///Logically we must assume that each entry in the list is a single element vector
void jaspTable::setColumnFromList(Rcpp::List column, int colIndex)
{
	std::vector<std::string> localRowNames = extractElementOrColumnNames(column);
	setRowNamesWhereApplicable(localRowNames);

	if(_data.size() <= colIndex)
		_data.resize(colIndex+1);
	_data[colIndex].clear();

	for(int row=0; row<column.size(); row++)
	{
		std::vector<Json::Value> jsonVec = jaspJson::RcppVector_to_VectorJson((Rcpp::RObject)column[row]);
		_data[colIndex].push_back(jsonVec.size() > 0 ? jsonVec[0u] : Json::nullValue);
	}
}

int jaspTable::equalizeColumnsLengths()
{
	if(_data.size() == 0)
		return 0;

	size_t maximumFoundColumnLength = 0;

	for(auto col : _data)
		maximumFoundColumnLength = std::max(maximumFoundColumnLength, col.size());

	for(auto col : _data)
		for(int row = col.size(); row < maximumFoundColumnLength; row++)
			col.push_back(Json::Value(Json::nullValue));

	return maximumFoundColumnLength;
}

Json::Value jaspTable::getCell(size_t col, size_t row)
{
	if(_data.size() <= col || _data[col].size() <= row)
		return Json::nullValue;
	return _data[col][row];
}

std::string	jaspTable::getCellFormatted(size_t col, size_t row)
{
	Json::Value val(getCell(col, row));

	std::string format = "";
	if(_colFormats.containsField(getColName(col)))
		format = _colFormats[getColName(col)];
	else if(_colFormats.rowCount() > col)
		format = _colFormats[col];

	if(val.isNull())
		return "";

	if(val.isString())
		return val.asString();

	if(val.isBool())
		return val.asBool() ? "true" : "false";

	if(format == "")
	{
		if(val.isInt())			return std::to_string(val.asInt());
		if(val.isDouble())
		{
			std::stringstream out;
			out.unsetf(std::ios_base::floatfield); //is std::defaultfloat for old compilers... RTools Im looking at you
			out << val.asDouble();
			return out.str();
		}
	}

	if(!val.isDouble() && !val.isInt())
		return "";

	//now, format != "" and the value is a number of some sort, lets apply the format!
	double value = val.isDouble() ? val.asDouble() : val.asInt();

	auto formats = stringSplit(format, ';');

	int		decPts	= -1,
			sigFig	= -1;
	double	pVal	= 0;
	bool	round	= false,
			log10	= false,
			prcnt	= false;

	std::string pValOri = "";

	for(auto & f : formats)
		if		(f == "~")		round = true;
		else if	(f == "log10")	log10 = true;
		else if(f == "pc")		prcnt = true;
		else if(f.find(':') != std::string::npos)
		{
			auto fmtval = stringSplit(f, ':');
			try
			{
				if		(fmtval[0] == "dp")	decPts	= std::stoi(fmtval[1]);
				else if	(fmtval[0] == "sf")	sigFig	= std::stoi(fmtval[1]);
				else if	(fmtval[0] == "p" )
				{
					pVal	= std::stod(fmtval[1]);
					pValOri = fmtval[1];
				}
				else
					std::cout << "unknown formatting option '" << fmtval[0] << "'" << std::endl;

			}
			catch(std::invalid_argument & e)	{}
			catch(std::out_of_range & e)		{}
		}
	else
		std::cout << "unknown formatting option '" << f << "'" << std::endl;

	if(log10)
		std::cout << "jaspTable doesnt know what to do with the formatting option 'log10', if you DO know, contact Joris Goosen or your local jaspResults-programmer..." << std::endl;

	if(prcnt)
	{
		if(sigFig > 0)
			prcnt = false;
		else
			value *= 100.0f;
	}

	if(pValOri != "" && pVal > value)
		return "p < " + pValOri;

	std::stringstream out;
	out.unsetf(std::ios_base::floatfield); //is std::defaultfloat for old compilers... RTools Im looking at you

	if(sigFig > 0)			out << std::scientific		<< std::setprecision(sigFig) << value;
	else if(decPts > 0)		out << std::fixed			<< std::setprecision(decPts) << value;
	else if(round)			out << std::round(value);
	else					out << value;

	if(prcnt)
		out << "%";

	return out.str();
}

std::vector<std::vector<std::string>> jaspTable::dataToRectangularVector(bool normalizeColLengths, bool normalizeRowLengths, bool onlySpecifiedColumns)
{
	size_t maxRow=0, maxCol=0;

	for(size_t col=0; col<_data.size(); col++)
	{
		if(!onlySpecifiedColumns || columnSpecified(col))
			maxCol++;

		maxRow = std::max(maxRow, _data[col].size());
	}

	std::vector<std::vector<std::string>> uit;

	uit.resize(maxCol);
	size_t colDst = 0;
	for(size_t colSrc=0; colSrc<_data.size() && colDst<maxCol; colSrc++)
	{
		if(!onlySpecifiedColumns || columnSpecified(colSrc))
		{
			uit[colDst].resize(maxRow);

			for(size_t row=0; row<maxRow; row++)
				uit[colDst][row] = getCellFormatted(colSrc, row);

			colDst++;
		}
	}

	if(normalizeColLengths)
		for(size_t col=0; col<maxCol; col++)
		{
			size_t maxLen = 0;

			for(size_t row=0; row<maxRow; row++)
				maxLen = std::max(uit[col][row].size(), maxLen);

			for(size_t row=0; row<maxRow; row++)
				stringExtend(uit[col][row], maxLen);
		}

	if(normalizeRowLengths)
		for(size_t row=0; row<maxRow; row++)
		{
			size_t maxLen = 0;

			for(size_t col=0; col<maxCol; col++)
				maxLen = std::max(uit[col][row].size(), maxLen);

			for(size_t col=0; col<maxCol; col++)
				stringExtend(uit[col][row], maxLen);
		}

	return uit;
}

std::vector<std::string> jaspTable::getDisplayableColTitles(bool normalizeLengths, bool onlySpecifiedColumns)
{
	std::vector<std::string> names;
	size_t maxLength = 0;

	for(size_t col=0; col<_data.size(); col++)
		if(!onlySpecifiedColumns || columnSpecified(col))
		{
			std::string name		= _colNames[col],
						showName	= getColName(col),
						title		= "";



			if(name != "" && _colTitles[name] != "")	title = _colTitles[name];
			else if(_colTitles[col] != "")				title = _colTitles[col];

			if(title != "")
				showName = title;

			maxLength = std::max(showName.size(), maxLength);

			names.push_back(showName);
		}

	if(normalizeLengths)
		for(auto & str : names)
			stringExtend(str, maxLength);

	return names;
}

std::vector<std::string> jaspTable::getDisplayableRowTitles(bool normalizeLengths)
{
	std::vector<std::string> names;
	size_t	maxLength	= 0,
			rowMax		= 0;

	for(size_t col=0; col<_data.size(); col++)
		rowMax = std::max(rowMax, _data[col].size());

	for(size_t row=0; row<rowMax; row++)
	{
		std::string name		= _rowNames[row],
					showName	= getRowName(row),
					title		= "";

		if(name != "" && _rowTitles[name] != "")	title = _rowTitles[name];
		else if(_colTitles[row] != "")				title = _rowTitles[row];

		if(title != "")
			showName = title;

		maxLength = std::max(showName.size(), maxLength);

		names.push_back(showName);
	}

	if(normalizeLengths)
		for(auto & str : names)
			stringExtend(str, maxLength);

	return names;
}

std::vector<std::vector<std::string>> jaspTable::transposeRectangularVector(const std::vector<std::vector<std::string>> & in)
{
	if(in.size() == 0)
		return in;

	std::vector<std::vector<std::string>> uit;

	uit.resize(in[0].size());

	for(auto & vec : uit)
		vec.resize(in.size());

	for(size_t col=0; col<in.size(); col++)
		for(size_t row=0; row<in[col].size(); row++)
			uit[row][col] = in[col][row];

	return uit;
}

std::map<std::string, std::map<size_t, size_t>> jaspTable::getOvertitleRanges(std::vector<std::string> names, std::map<std::string,std::string> overtitles)
{
	std::map<std::string, std::map<size_t, size_t>> overtitleSpread;

	for(size_t top=0; top<names.size(); top++)
	{

		std::string trimmedName	= stringRemove(names[top]),
					overTitle	= overtitles.count(trimmedName) > 0 ? overtitles[trimmedName] : "";

		if(overtitleSpread.count(overTitle) == 0)
			overtitleSpread[overTitle][top] = top;
		else
		{
			bool foundIt = false;

			for(size_t begin=0; begin < top; begin++)
				if(overtitleSpread[overTitle].count(begin) > 0 && overtitleSpread[overTitle][begin] == top - 1)
				{
					overtitleSpread[overTitle][begin] = top;
					foundIt = true;
					break;
				}

			if(!foundIt)
				overtitleSpread[overTitle][top] = top;
		}
	}

	return overtitleSpread;
}

void jaspTable::rectangularDataWithNamesToString(std::stringstream & out, std::string prefix, std::vector<std::vector<std::string>> vierkant, std::vector<std::string> sideNames, std::vector<std::string> topNames, std::map<std::string,std::string> sideOvertitles, std::map<std::string,std::string> topOvertitles)
{
	if(vierkant.size() == 0)
		return;

	size_t	sideOvertitleSpace = 0;

	for(auto & keyval : sideOvertitles)
		sideOvertitleSpace = std::max(sideOvertitleSpace, keyval.second.size());

	if(sideOvertitleSpace > 0) sideOvertitleSpace += 1;

	std::vector<std::string> sideOverTitleRow;
	std::string prevSideTitle = "";
	for(auto & sideName : sideNames)
	{
		std::string sideTrim = stringRemove(sideName);

		if(sideOvertitles.count(sideTrim) > 0)
		{
				std::string sideTitle = sideOvertitles[sideTrim];
				if(sideTitle != prevSideTitle)
				{
					sideOverTitleRow.push_back(sideTitle + " ");
					prevSideTitle = sideTitle;
				}
				else
					sideOverTitleRow.push_back(std::string(sideOvertitleSpace, ' '));
		}
		else
			sideOverTitleRow.push_back(std::string(sideOvertitleSpace, ' '));
	}


	//If there are overtitles above the top we need to make sure they fit, so we expand the topnames if needed.
	std::map<std::string, std::map<size_t, size_t>> overtitleSpread = getOvertitleRanges(topNames, topOvertitles);

	for(auto & overtitleRange : overtitleSpread)
		for(auto & range : overtitleRange.second)
		{
			std::string overtitle = overtitleRange.first + "  "; //extra "  " space to make it align better
			size_t topNamesLength = 0;

			do
			{
				topNamesLength	= 0;

				for(size_t tops = range.first; tops <= range.second; tops++)
				{
					topNamesLength += topNames[tops].size();

					if(tops > range.first)
						topNamesLength += 3; // because of the ' | ' added to the print later
				}

				if(topNamesLength < overtitle.size())
				{
					for(size_t tops = range.first; tops <= range.second; tops++)
						topNames[tops] += ' ';
					topNamesLength += 3;
				}
			}
			while(topNamesLength < overtitle.size());
		}

	//we want to display rownames above the "columns of cells", which means they must fit!
	//So either we make all cols of a row bigger to correspond to the rowname size or vice versa!
	for(size_t row=0; row<topNames.size(); row++)
		if(topNames[row].size() > vierkant[0][row].size())
			for(size_t col=0; col<vierkant.size(); col++)
				stringExtend(vierkant[col][row], topNames[row].size());
		else if(topNames[row].size() < vierkant[0][row].size())
			stringExtend(topNames[row], vierkant[0][row].size());

	size_t extraSpaceSide = sideNames[0].size() + sideOvertitleSpace;

	//lets print the topOvertitles
	{
		out << prefix << std::string(extraSpaceSide, ' ') << "    ";
		size_t overTitleEnd = 0, topNameEnd = 0;

		for(size_t row=0; row<topNames.size(); row++)
		{
			std::string topName = topNames[row];
			std::string trimmed = stringRemove(topName);

			if(topOvertitles.count(trimmed) > 0)
			{
				std::string overTitle = topOvertitles[trimmed];
				if(overtitleSpread.count(overTitle) > 0 && overtitleSpread[overTitle].count(row) > 0)
				{
					out << overTitle;
					overTitleEnd += overTitle.size();
				}
			}

			topNameEnd += topName.size() + 3; //3 because of " | "

			if(overTitleEnd < topNameEnd)
			{
				out << std::string(topNameEnd - overTitleEnd, ' ');
				overTitleEnd = topNameEnd;
			}
		}
		out << "  \n";
	}

	//lets print the topnames
	out << prefix << std::string(extraSpaceSide, ' ') << "    ";
	for(size_t row=0; row<topNames.size(); row++)
		out << (row>0? "   " : "") << topNames[row];
	out << "  \n";

	//lets create a nice reusable layer of ----
	std::stringstream colSep;

	colSep << prefix << std::string(extraSpaceSide, ' ') << "  |-";

	for(size_t row=0; row<vierkant[0].size(); row++)
		colSep << (row>0? "-|-" : "") << std::string(vierkant[0][row].size(), '-');
	colSep << "-|\n";

	//then the actual columns X rows
	for(size_t col=0; col<vierkant.size(); col++)
	{
		//put the side overtitle here
		out << colSep.str();
		out << prefix << sideOverTitleRow[col] << sideNames[col] << "  | ";

		for(size_t row=0; row<vierkant[col].size(); row++)
			out << (row>0? " | " : "") << vierkant[col][row];

		out << " |\n";
	}

	out << colSep.str();
}

std::map<std::string, std::string> jaspTable::getOvertitlesMap()
{
	std::map<std::string, std::string> map;

	for(size_t col=0; col<_data.size(); col++)
	{
		std::string colName = getColName(col);
		if(_colOvertitles.containsField(colName))
			map[stringRemove(colName)] = _colOvertitles[colName];
	}

	return map;
}

std::string jaspTable::dataToString(std::string prefix)
{
	std::stringstream out;

	std::vector<std::vector<std::string>>	vierkant = dataToRectangularVector(!_transposeTable, _transposeTable, _showSpecifiedColumnsOnly);
	std::vector<std::string>				colNames = getDisplayableColTitles(true, _showSpecifiedColumnsOnly),
											rowNames = getDisplayableRowTitles();

	out << prefix << "status: " << _status << "\n";

	if(_error != "" || _errorMessage != "")
	{
		out << prefix;
		if(_error		 != "") out << "error: '" << _error << "'";
		if(_errorMessage != "") out << (_error != "" ? " msg: '" : "errormessage: '") << _errorMessage << "'";
		out << "\n";
	}
	else
	{
		if(_transposeTable) rectangularDataWithNamesToString(out, prefix, vierkant,								colNames, rowNames, getOvertitlesMap(),	{});
		else				rectangularDataWithNamesToString(out, prefix, transposeRectangularVector(vierkant),	rowNames, colNames,	{},						getOvertitlesMap());
	}


	if(_footnotes.size() > 0)
	{
		out << "\n" << prefix << "footnotes:   \n";
		for(Json::Value::UInt i=0; i<_footnotes.size(); i++)
		{
			std::string sym = _footnotes[i]["symbol"].asString() ;
			out << prefix << "\t" << (sym == "" ? "" : "(" + sym  + ") " ) << "'" << _footnotes[i]["text"].asString() << "'\n";
		}
	}

	return out.str();
}

std::string jaspTable::toHtml()
{
	std::stringstream out;

	std::vector<std::vector<std::string>>	vierkant = dataToRectangularVector(false, false, _showSpecifiedColumnsOnly);
	std::vector<std::string>				colNames = getDisplayableColTitles(false, _showSpecifiedColumnsOnly),
											rowNames = getDisplayableRowTitles(false);
	out		<< "<div class=\"status " << _status << " jaspTable\">\n"
			<< htmlTitle() << "\n";

	if(_error != "" || _errorMessage != "")
	{
		out << "<p class=\"error\">\n";
		if(_error		 != "") out << "error: <i>'" << _error << "'</i>";
		if(_errorMessage != "") out << (_error != "" ? " msg: <i>'" : "errormessage: <i>'") << _errorMessage << "'</i>";
		out << "\n</p>";
	}
	else
	{
		if(_transposeTable) rectangularDataWithNamesToHtml(out, vierkant,								colNames, rowNames, getOvertitlesMap(),	{});
		else				rectangularDataWithNamesToHtml(out, transposeRectangularVector(vierkant),	rowNames, colNames,	{},						getOvertitlesMap());
	}


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

void jaspTable::rectangularDataWithNamesToHtml(std::stringstream & out, std::vector<std::vector<std::string>> vierkant, std::vector<std::string> sideNames, std::vector<std::string> topNames, std::map<std::string,std::string> sideOvertitles, std::map<std::string,std::string> topOvertitles)
{
	if(vierkant.size() == 0)
	{
		out << "\t<table>empty</table>\n";
		return;
	}

	auto	topOvertitleSpread	= getOvertitleRanges(topNames,	topOvertitles),
			sideOvertitleSpread = getOvertitleRanges(sideNames, sideOvertitles);

	bool	topOvertitlesPresent  = topOvertitleSpread.size()  > (topOvertitleSpread.count("")  > 0 ? 1 : 0),
			sideOvertitlesPresent = sideOvertitleSpread.size() > (sideOvertitleSpread.count("") > 0 ? 1 : 0);

	out << "\t<table>\n";

	if(topOvertitlesPresent)
	{
		out << "\t\t"   "<tr>\n"
			   "\t\t\t" "<th rowspan=\"2\" colspan=\"" << (sideOvertitlesPresent ? 2 : 1) << "\"></th>" "\n";

		for(size_t top=0; top<topNames.size(); top++)
		{
			std::string topName		= topNames[top],
						trimmed		= stringRemove(topName),
						overtitle	= topOvertitles.count(trimmed) > 0 ? topOvertitles[trimmed] : "";

			if(topOvertitleSpread.count(overtitle) > 0 && topOvertitleSpread[overtitle].count(top) > 0)
				out << "\t\t\t" "<th colspan=\"" <<  topOvertitleSpread[overtitle][top] - (top - 1) << "\">" << overtitle << "</th>" "\n";

		}
		out << "\t\t" "</tr>" "\n";
	}

	out << "\t\t" "<tr>\n";
	for(auto & topName : topNames)
			out << "\t\t\t" "<th>" << topName << "</th>" "\n";
	out << "\t\t" "</tr>" "\n";

	//then the actual columns X rows
	for(size_t side=0; side<vierkant.size(); side++)
	{
		out << "\t\t" "<tr>" "\n";

		std::string name		= sideNames[side],
					trimmed		= stringRemove(name),
					overtitle	= topOvertitles.count(trimmed) > 0 ? topOvertitles[trimmed] : "";

		if(sideOvertitlesPresent && sideOvertitleSpread.count(overtitle) > 0 && sideOvertitleSpread[overtitle].count(side) > 0)
			out << "\t\t\t" "<th rowspan=\"" << sideOvertitleSpread[overtitle][side] - (side - 1) << "\">" << overtitle << "</th>" "\n";

		out << "\t\t\t" "<th>" << name << "</th>" "\n";

		for(size_t top=0; top<vierkant[side].size(); top++)
			out << "\t\t\t" "<td>" << vierkant[side][top] << "</td>" "\n";

		out << "\t\t" "</tr>" "\n";
	}

	out << "\t</table>\n";
}

void jaspTable::addFootnote(Rcpp::RObject message, Rcpp::RObject symbol, Rcpp::RObject col_names, Rcpp::RObject row_names)
{
	if(message.isNULL())
		Rf_error("One would expect a footnote to at least contain a message..");

	std::vector<Json::Value> colNames = jaspJson::RcppVector_to_VectorJson(col_names, false);
	std::vector<Json::Value> rowNames = jaspJson::RcppVector_to_VectorJson(row_names, false);

	Json::Value note(Json::objectValue);

	note["symbol"]	= symbol.isNULL()	? "" :	Rcpp::as<std::string>(symbol);
	note["text"]	= Rcpp::as<std::string>(message);
	note["cols"]	= colNames.size() == 0 ? Json::nullValue : jaspJson::VectorJson_to_ArrayJson(colNames);
	note["rows"]	= rowNames.size() == 0 ? Json::nullValue : jaspJson::VectorJson_to_ArrayJson(rowNames);

	_footnotes.append(note);
}

/*
void jaspTable::combineCells(Rcpp::map_named_args & named_args)
{
	///we are going to pretend that the arguments in R would be: combineColums(colnames=c(""), title="", overwrite=FALSE, removeSeparator=FALSE) or combineRows(rownames=c(""), title="", overwrite=FALSE, removeSeparator=FALSE)
	std::queue<Rcpp::RObject> unnamed_args = extract_unnamed_args(named_args);

	Rcpp::RObject colNames			= extractFieldOrFirstUnnamed("colnames",			named_args, unnamed_args);
	Rcpp::RObject colOvertitles		= extractFieldOrFirstUnnamed("colOvertitles",		named_args, unnamed_args);
	Rcpp::RObject rowNames			= extractFieldOrFirstUnnamed("rownames",			named_args, unnamed_args);
	Rcpp::RObject rowOvertitles		= extractFieldOrFirstUnnamed("rowOvertitles",		named_args, unnamed_args);
	Rcpp::RObject title				= extractFieldOrFirstUnnamed("title",				named_args, unnamed_args);
	Rcpp::RObject overwrite			= extractFieldOrFirstUnnamed("overwrite",			named_args, unnamed_args);
	Rcpp::RObject removeSeparator	= extractFieldOrFirstUnnamed("removeSeparator",		named_args, unnamed_args);
	Rcpp::RObject name				= extractFieldOrUseDefault("title",					named_args, title);

	std::vector<Json::Value> vecJsonColNames		= jaspJson::RcppVector_to_VectorJson(colNames,		false);
	std::vector<Json::Value> vecJsonRowNames		= jaspJson::RcppVector_to_VectorJson(rowNames,		false);
	std::vector<Json::Value> vecJsonColOvertitles	= jaspJson::RcppVector_to_VectorJson(colOvertitles, false);
	std::vector<Json::Value> vecJsonRowOvertitles	= jaspJson::RcppVector_to_VectorJson(rowOvertitles, false);


	if(	vecJsonColNames.size() == 0 && vecJsonColNames.size() == 0 && vecJsonColOvertitles.size() == 0 && vecJsonRowOvertitles.size() == 0)
		return; //It doesnt make a lot of sense to combine nothing right?

	_colRowCombinations.push_back(
		jaspColRowCombination(
			name.isNULL()				? ""	: Rcpp::as<std::string>(name),
			title.isNULL()				? ""	: Rcpp::as<std::string>(title),
			overwrite.isNULL()			? false	: Rcpp::as<bool>(overwrite),
			removeSeparator.isNULL()	? false	: Rcpp::as<bool>(removeSeparator),
			jaspJson::VectorJson_to_ArrayJson(vecJsonColNames),
			jaspJson::VectorJson_to_ArrayJson(vecJsonRowNames),
			jaspJson::VectorJson_to_ArrayJson(vecJsonColOvertitles),
			jaspJson::VectorJson_to_ArrayJson(vecJsonRowOvertitles)
		)
	);

}
*/
Json::Value jaspTable::dataEntry()
{
	Json::Value dataJson(jaspObject::dataEntry());

	dataJson["title"]				= _title;

	dataJson["name"]				= getUniqueNestedName();
	dataJson["footnotes"]			= _footnotes;
	dataJson["schema"]				= schemaJson();

	dataJson["data"]				= rowsJson();
	dataJson["casesAcrossColumns"]	= _transposeTable;
	dataJson["overTitle"]			= _transposeWithOvertitle;

	dataJson["status"]				= _error == "" ? _status : "error";

	if(_error != "")
	{
		dataJson["error"]					= Json::objectValue;
		dataJson["error"]["type"]			= _error;
		dataJson["error"]["errorMessage"]	= _errorMessage;
	}

	return dataJson;
}



Json::Value	jaspTable::schemaJson()
{
    Json::Value schema(Json::objectValue);
	Json::Value fields(Json::arrayValue);

	for(int col=0; col<_colNames.rowCount(); col++)
	{
		Json::Value field(Json::objectValue);

		std::string colName		= getColName(col);
		std::string colTitle	= _colTitles.containsField(colName) ? _colTitles[colName] : (_colTitles[col] != "" ? _colTitles[col] : colName);
		std::string colFormat	= _colFormats.containsField(colName) ? _colFormats[colName] : (_colFormats[col] != "" ? _colFormats[col] : "");
		std::string colType		= deriveColumnType(col);

		field["name"]	= colName;
		field["title"]	= colTitle;

		field["type"]	= getColType(col);

		if(_colOvertitles.containsField(colName))
			field["overTitle"] = _colOvertitles[colName];

		if(_colCombines.containsField(colName))
			field["combine"] = _colCombines[colName];

		if(colFormat != "")
			field["format"]	= colFormat;

		if(colName[0] != '.' && (!_showSpecifiedColumnsOnly || _specifiedColumns.count(_colNames[col]) > 0))
			fields.append(field);

	}

    schema["fields"] = fields;
    return schema;
}

Json::Value	jaspTable::rowsJson()
{
	Json::Value rows(Json::arrayValue);

	bool keepGoing = true;
	for(int row=0; keepGoing; row++)
	{
		Json::Value aRow(Json::objectValue);
		bool aColumnKeepsGoing = false;

		for(int col=0; col<_data.size(); col++)
		{
			if(_data[col].size() > row)
				aColumnKeepsGoing = true;

			aRow[getColName(col)] = getCell(col, row);
		}

		if(aColumnKeepsGoing)
			rows.append(aRow);
		else
			keepGoing = false;
	}

	return rows;
}

std::string jaspTable::deriveColumnType(int col)
{
	if(col >= _data.size())
		return "null";

	Json::ValueType workingType = Json::nullValue;
	const std::string variousType = "various";

	for(auto & cell : _data[col])
		switch(workingType)
		{
		case Json::nullValue:
			workingType = cell.type();
			break;

		case Json::stringValue:
		case Json::booleanValue:
			if(cell.type() != workingType)
				return variousType;
			break;

		case Json::intValue:
		case Json::uintValue:
			if(cell.type() == Json::realValue)
				workingType = Json::realValue;
			else if(cell.type() != workingType)
				return variousType;
			break;

		case Json::realValue:
			if(!(cell.type() == workingType || cell.type() == Json::intValue || cell.type() == Json::uintValue))
				return variousType;
			break;

		default:
			return "composite"; //arrays and objects are not really supported as cells at the moment but maybe we could add that in the future?
		}

	switch(workingType)
	{
	case Json::nullValue:		return "null";
	case Json::stringValue:		return "string";
	case Json::booleanValue:	return "logical";
	case Json::intValue:
	case Json::uintValue:		return "integer";
	case Json::realValue:		return "number";
	default:					return "unknown";
	}
}

std::string jaspTable::getColType(size_t col)
{
	std::string colName = getColName(col);

	if(_colTypes[colName] != "")	return _colTypes[colName];
	if(_colTypes[col] != "")		return _colTypes[col];

	return deriveColumnType(col);
}

///Going to assume it is called like addColumInfo(name=NULL, title=NULL, type=NULL, format=NULL, combine=NULL)
void jaspTable::addColumnInfo(Rcpp::RObject name, Rcpp::RObject title, Rcpp::RObject type, Rcpp::RObject format, Rcpp::RObject combine, Rcpp::RObject overtitle)
{
	std::string colName = name.isNULL() ? defaultColName(_colNames.rowCount()) : Rcpp::as<std::string>(name);
	_specifiedColumns.insert(colName);

	_colNames.add(colName);

	std::string lastAddedColName = getColName(_colNames.rowCount() - 1);

	if(!title.isNULL())		_colTitles[lastAddedColName]		= Rcpp::as<std::string>(title);
	if(!type.isNULL())		_colTypes[lastAddedColName]			= Rcpp::as<std::string>(type);
	if(!format.isNULL())	_colFormats[lastAddedColName]		= Rcpp::as<std::string>(format);
	if(!combine.isNULL())	_colCombines[lastAddedColName]		= Rcpp::as<bool>(combine);
	if(!overtitle.isNULL())	_colOvertitles[lastAddedColName]	= Rcpp::as<std::string>(overtitle);
}


Json::Value jaspTable::convertToJSON()
{
	Json::Value obj		= jaspObject::convertToJSON();

	obj["status"]					= _status;
	obj["error"]					= _error;
	obj["errorMessage"]				= _errorMessage;
	obj["footnotes"]				= _footnotes;
	obj["transposeTable"]			= _transposeTable;
	obj["transposeWithOvertitle"]	= _transposeWithOvertitle;
	obj["showSpecifiedColumnsOnly"]	= _showSpecifiedColumnsOnly;

	obj["colNames"]				= _colNames.convertToJSON();
	obj["colTypes"]				= _colTypes.convertToJSON();
	obj["rowNames"]				= _rowNames.convertToJSON();
	obj["rowTitles"]			= _rowTitles.convertToJSON();
	obj["colTitles"]			= _colTitles.convertToJSON();
	obj["colOvertitles"]		= _colOvertitles.convertToJSON();
	obj["colFormats"]			= _colFormats.convertToJSON();
	obj["colCombines"]			= _colCombines.convertToJSON();



	Json::Value dataColumns(Json::arrayValue);

	for(auto & col : _data)
	{
		Json::Value dataRows(Json::arrayValue);

		for(auto & row : col)
			dataRows.append(row);

		dataColumns.append(dataRows);
	}

	obj["data"]	= dataColumns;

	Json::Value colRowCombos(Json::arrayValue);

	for(auto & colRow : _colRowCombinations)
		colRowCombos.append(colRow.convertToJSON());

	obj["colRowCombinations"] = colRowCombos;

	obj["specifiedColumns"] = Json::arrayValue;
	for(const std::string & specifiedColumnName : _specifiedColumns)
		obj["specifiedColumns"].append(specifiedColumnName);

	return obj;
}

void jaspTable::convertFromJSON_SetFields(Json::Value in)
{
	jaspObject::convertFromJSON_SetFields(in);


	_status						= in.get("status",						"null").asString();
	_error						= in.get("error",						"null").asString();
	_errorMessage				= in.get("errorMessage",				"null").asString();
	_footnotes					= in.get("footnotes",					Json::arrayValue);
	_transposeTable				= in.get("transposeTable",				false).asBool();
	_transposeWithOvertitle		= in.get("transposeWithOvertitle",		false).asBool();
	_showSpecifiedColumnsOnly	= in.get("showSpecifiedColumnsOnly",	false).asBool();


	_colNames.convertFromJSON_SetFields(		in.get("colNames",		Json::objectValue));
	_colTypes.convertFromJSON_SetFields(		in.get("colTypes",		Json::objectValue));
	_rowNames.convertFromJSON_SetFields(		in.get("rowNames",		Json::objectValue));
	_rowTitles.convertFromJSON_SetFields(		in.get("rowTitles",		Json::objectValue));
	_colTitles.convertFromJSON_SetFields(		in.get("colTitles",		Json::objectValue));
	_colFormats.convertFromJSON_SetFields(		in.get("colFormats",	Json::objectValue));
	_colCombines.convertFromJSON_SetFields(		in.get("colCombines",	Json::objectValue));
	_colOvertitles.convertFromJSON_SetFields(	in.get("colOvertitles",	Json::objectValue));

	_data.clear();
	Json::Value dataColumns(in.get("data",	Json::arrayValue));
	for(auto & col : dataColumns)
	{
		std::vector<Json::Value> newCol;

		for(auto & rowElem : col)
			newCol.push_back(rowElem);

		_data.push_back(newCol);
	}

	_colRowCombinations.clear();
	Json::Value colRowCombos(in.get("colRowCombinations",	Json::arrayValue));

	for(auto & colRowCombo : colRowCombos)
		_colRowCombinations.push_back(jaspColRowCombination(colRowCombo));

	 _specifiedColumns.clear();
	for(Json::Value & specifiedColumnName : in.get("specifiedColumns", Json::arrayValue))
		_specifiedColumns.insert(specifiedColumnName.asString());
}



#ifndef IDE_PROJECTS_H
#define IDE_PROJECTS_H

#include <string>

class IProjectFormat
{
public:
	virtual ~IProjectFormat() {}

	virtual void Open(const char* filename) = 0;

	virtual std::string GetBuildCommand() = 0;

	virtual std::string GetExecutablePath() { return ""; }

	virtual std::string GetProjectFolder() = 0;

	virtual std::string GetLineNumberRegex() = 0;

	virtual std::string GetColumnNumberRegex() { return ""; }

	virtual std::string GetFileNameRegex() = 0;

	virtual std::vector<std::string> GetDependencies() { return std::vector<std::string>(); }
};

class JetProject : public IProjectFormat
{
	std::string filename;
public:

	virtual void Open(const char* filename)
	{
		this->filename = filename;
	}

	virtual std::string GetBuildCommand()
	{
		return "jet build " + this->GetProjectFolder() + " --linker=ld";
	}

	virtual std::string GetExecutablePath()
	{
		std::string project_name = this->GetProjectFolder();
		int off = project_name.find_last_of('\\');
		project_name = project_name.substr(off + 1);
		return this->GetProjectFolder() + "/build/" + project_name + ".exe";
	}

	virtual std::string GetProjectFolder()
	{
		std::string folder = filename;
		int off = folder.find_last_of('\\');
		folder = folder.substr(0, off);
		return folder;
	}

	virtual std::string GetLineNumberRegex()
	{
		return "(?:\\[\\w*\\] [\\w\\.]* )\\d*";// "\d+(?=:)";//one per line
	}

	virtual std::string GetColumnNumberRegex()
	{
		return "(?:\\d)\\d+[^:]";
	}

	virtual std::string GetFileNameRegex()
	{
		return "[\\w/]+.jet";
	}
};


class CMakeProject : public IProjectFormat
{
	std::string filename;
public:

	virtual void Open(const char* filename)
	{
		this->filename = filename;
	}

	virtual std::string GetBuildCommand()
	{
		return "cmake --build " + this->GetProjectFolder();
	}

	virtual std::string GetExecutablePath()
	{
		std::string project_name = this->GetProjectFolder();
		int off = project_name.find_last_of('\\');
		project_name = project_name.substr(off + 1);
		return this->GetProjectFolder() + "/build/" + project_name + ".exe";
	}

	virtual std::string GetProjectFolder()
	{
		std::string folder = filename;
		int off = folder.find_last_of('\\');
		folder = folder.substr(0, off);
		return folder;
	}

	virtual std::string GetLineNumberRegex()
	{
		return "\\d+(?=:)";//one per line
	}

	virtual std::string GetColumnNumberRegex()
	{
		return "(?:\\d)\\d+[^:]";
	}

	virtual std::string GetFileNameRegex()
	{
		return "[\\w/]+.jet";
	}
};

#endif

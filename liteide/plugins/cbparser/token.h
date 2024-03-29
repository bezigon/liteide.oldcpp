/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef TOKEN_H
#define TOKEN_H
/*
#include <wx/string.h>
#include <wx/dynarray.h>
#include <wx/file.h>
#include <wx/thread.h>
#include <wx/stream.h>
#include <settings.h>

#include "blockallocated.h"
#include <globals.h>
*/
#include "wxhelper.h"
#include "vfc_token.h"
#include "blockallocated.h"
#include "searchtree.h"
#include <deque>


using namespace std;

class Token;
class TokensTree;

static wxCriticalSection s_MutexProtection;

enum FileParsingStatus
{
    fpsNotParsed = 0,
    fpsAssigned,
    fpsBeingParsed,
    fpsDone
};

typedef std::vector<wxString> wxArrayString;
typedef vector<Token*> TokenArray;
typedef vector<Token*> TokenList;
typedef deque<int> TokenIdxList;
typedef set<int, less<int> > TokenIdxSet;
typedef SearchTree<TokenIdxSet> TokenSearchTree;
typedef BasicSearchTree TokenFilenamesMap;
typedef map<size_t,TokenIdxSet,less<size_t> > TokenFilesMap;
typedef map<size_t,FileParsingStatus,less<size_t> > TokenFilesStatus;
typedef set<size_t,less<size_t> > TokenFilesSet;

enum TokenScope
{
    tsUndefined = 0,
    tsPrivate,
    tsProtected,
    tsPublic
};

enum TokenKind
{
    tkClass         = 0x0001,
    tkNamespace     = 0x0002,
    tkTypedef       = 0x0004, // typedefs are stored as classes inheriting from the typedef'd type (taking advantage of existing inheritance code)
    tkConstructor   = 0x0008,
    tkDestructor    = 0x0010,
    tkFunction      = 0x0020,
    tkVariable      = 0x0040,
    tkEnum          = 0x0080,
    tkEnumerator    = 0x0100,
    tkPreprocessor  = 0x0200,
	tkMapping	    = 0x0400, 	//vfc add the enum

    // convenient masks
    tkAnyContainer  = tkClass | tkNamespace | tkTypedef,
    tkAnyFunction   = tkFunction | tkConstructor | tkDestructor,

    tkUndefined     = 0xFFFF,
};


class Token  : public BlockAllocated<Token, 10000>, public vfc_token
{
    friend class TokensTree;
public:
	virtual  size_t	get_string(int vt, wchar_t * text, size_t cch)
	{ 
		CString str;
		switch(vt)
		{
		case vt_Type:
			str = m_Type;
			break;
		case vt_ActualType:
			str = m_ActualType;
			break;
		case vt_Name:
			 str = m_Name;
			 break;
		case vt_Args:
			str = m_Args;
			break;
		case vt_RealArgs:
			str = m_RealArgs;
			break;
		case vt_AncestorsString:
			str = m_AncestorsString;
			break;
		case vt_FileName:
			str = GetFilename();
			break;
		case vt_ImplFileName:
			str =  GetImplFilename();
			break;
		default:
			str = _T("");
			break;
		}
		if (str.GetLength() == 0)
			return 0;
		int max = cch >= str.GetLength() ? cch : str.GetLength();
		::wcsncpy(text,str,max);
		return max;
	}
	virtual const unsigned int get_uint(int vt) { 
		switch(vt)
		{
		case vt_File:
			return m_File;
		case vt_Line:
			return m_Line;
		case vt_ImplFile:
			return m_ImplFile;
		case vt_ImplLine:
			return m_ImplLine;
		case vt_ImplLineStart:
			return m_ImplLineStart;
		case vt_ImplLineEnd:
			return m_ImplLineEnd;
		case vt_Scope:
			return m_Scope;
		case vt_TokenKind:
			return m_TokenKind;
		case vt_IsOperator:
			return m_IsOperator;
		case vt_IsLocal:
			return m_IsLocal;
		case vt_IsTemp:
			return m_IsTemp;
		case vt_IsConst:
			return m_IsConst;
		case vt_ParentIndex:
			return m_ParentIndex;
		case vt_Index:
			return m_Self;
		}
		return 0; 
	}
	virtual bool get_token_idx(int vt, i_vfc_int_array * pti) { 
		TokenIdxSet * pset = NULL;
		switch(vt)
		{
		case vt_Children:
			pset = &m_Children;
			break;
		case vt_Ancestors:
			pset = &m_Ancestors;
			break;
		case vt_DirectAncestors:
			pset =  &m_DirectAncestors;
			break;
		case vt_Descendants:
			pset = &m_Descendants;
			break;
		}
		if (pset == NULL || pti == NULL)
			return false;
		for (TokenIdxSet::iterator it = pset->begin(); it != pset->end(); it++)
		{
			pti->push_back(*it);
		}
		return true; 
	}

	virtual vfc_token_tree * get_tree()
	{
		return (vfc_token_tree*)m_pTree;
	}
	virtual vfc_token * get_parent()
	{
		return GetParentToken();
	}
	virtual int get_update()
	{
		return m_Update;
	}
	virtual void set_update(int value)
	{
		m_Update = value;
	}
	virtual void add_child(int id)
	{
		AddChild(id);
	}
	virtual void remove_child(int id)
	{
	}
		int	m_Update;
    public:
        Token();
        Token(const wxString& name, unsigned int file, unsigned int line);
        ~Token();

        void AddChild(int child);
        void RemoveChild(int child);
        wxString GetNamespace() const;
        bool InheritsFrom(int idx) const;
        wxString DisplayName() const;
        wxString GetTokenKindString() const;
        wxString GetTokenScopeString() const;
        wxString GetFilename() const;
        wxString GetImplFilename() const;
        inline unsigned long GetTicket() const { return m_Ticket; }
        bool MatchesFiles(const TokenFilesSet& files);

        bool SerializeIn(wxInputStream* f);
        bool SerializeOut(wxOutputStream* f);
        int GetSelf() { return m_Self; } // current index in the tree
        wxString GetParentName();
        Token* GetParentToken();
        TokensTree* GetTree() { return m_pTree; }

        wxString m_Type; // this is the return value (if any): e.g. const wxString&
        wxString m_ActualType; // this is what the parser believes is the actual return value: e.g. wxString
        wxString m_Name;
        wxString m_Args;
		wxString m_RealArgs;		// (int * a, int & b) -> (int*,int&)
        wxString m_AncestorsString; // all ancestors comma-separated list
        unsigned int m_File;
        unsigned int m_Line;
        unsigned int m_ImplFile;
        unsigned int m_ImplLine; // where the token was met
        unsigned int m_ImplLineStart; // if token is impl, opening brace line
        unsigned int m_ImplLineEnd; // if token is impl, closing brace line
        TokenScope m_Scope;
        TokenKind m_TokenKind;
        bool m_IsOperator;
        bool m_IsLocal; // found in a local file?
        bool m_IsTemp; // if true, the tree deletes it in FreeTemporaries()
        bool m_IsConst;    // the member method is const (yes/no)

        int m_ParentIndex;
        TokenIdxSet m_Children;
        TokenIdxSet m_Ancestors;
        TokenIdxSet m_DirectAncestors;
        TokenIdxSet m_Descendants;

        wxArrayString m_Aliases; // used for namespace aliases

        void* m_pUserData; // custom user-data (the classbrowser expects it to be a pointer to a cbProject)
    protected:
        TokensTree* m_pTree;
        int m_Self; // current index in the tree
        unsigned long m_Ticket;
        static unsigned long GetTokenTicket();

    private:
};

class TokensTree : public vfc_token_tree
{
public:
	virtual vfc_token *		 get_token(int idx)
	{
		return GetTokenAt(idx);
	}
	virtual bool get_token_array(i_vfc_token_array * par)
	{
		if (par == NULL)
			return false;
		for (TokenList::iterator it = m_Tokens.begin(); it != m_Tokens.end(); it++)
		{
			par->push_back(*it);
		}
		return true;
	}
	virtual bool get_token_files(i_vfc_int_array * par)
	{
		if (par == NULL)
			return false;
		for (TokenFilesMap::iterator it = m_FilesMap.begin(); it != m_FilesMap.end(); it++)
		{
			par->push_back(it->first);
		}
		return true;
	}
	virtual const wchar_t * get_token_file_name(int idx)
	{
		TokenIdxSet & file = m_FilesMap[idx];
		if (file.size() <= 0)
			return _T("");
		Token * tk = at(*file.begin());
		return tk->GetFilename();
	}
	virtual bool get_token_file_idx(int idx, i_vfc_int_array * par)
	{
		if (par == NULL)
			return false;
		TokenIdxSet & file = m_FilesMap[idx];
		for (TokenIdxSet::iterator it = file.begin(); it != file.end(); it++)
		{
			par->push_back(*it);
		}
		return true;
	}
	virtual int add_token(int parent_id,
		const wchar_t * name, 
		const wchar_t * type,
		const wchar_t * args,
		const wchar_t * body, 
		int scope, int kind)
	{
		Token * tk = new Token();
		tk->m_Name = name;
		tk->m_Type = type;
		tk->m_Args = args;
		tk->m_Scope = (TokenScope)scope;
		tk->m_TokenKind = (TokenKind)kind;
		return 0;
	}
    public:
        static const wxString s_version;
        TokensTree();
        inline void Clear() { clear(); }

        // STL compatibility functions
        void clear();
        inline Token* operator[](int idx) { return GetTokenAt(idx); }
        inline Token* at(int idx) { return GetTokenAt(idx); }
        size_t size();
        size_t realsize();
        inline bool empty() { return size()==0; }
        int insert(Token* newToken);
        int insert(int loc, Token* newToken);
        int erase(int loc);
        void erase(Token* oldToken);

        // Token specific functions
        void RecalcFreeList();
        void RecalcData();
        int TokenExists(const wxString& name, int parent, short int kindMask);
        size_t FindMatches(const wxString& s,TokenIdxSet& result,bool caseSensitive,bool is_prefix, int kindMask = 0xffff);
        size_t FindTokensInFile(const wxString& file, TokenIdxSet& result, short int kindMask);
        void RemoveFile(const wxString& filename);
        void RemoveFile(int index);
        void FreeTemporaries();
        virtual ~TokensTree();

        // Parsing related functions

        size_t GetFileIndex(const wxString& filename);
        const wxString GetFilename(size_t idx) const;
        size_t ReserveFileForParsing(const wxString& filename,bool preliminary = false);
        void FlagFileForReparsing(const wxString& filename);
        void FlagFileAsParsed(const wxString& filename);
        bool IsFileParsed(const wxString& filename);

        void MarkFileTokensAsLocal(const wxString& filename, bool local = true, void* userData = 0);
        void MarkFileTokensAsLocal(size_t file, bool local = true, void* userData = 0);

        TokenList m_Tokens; /// Contains the pointers to all the tokens
        TokenSearchTree m_Tree; /** Tree containing the indexes to the tokens
          (the indexes will be used on m_Tokens) */

        TokenFilenamesMap m_FilenamesMap; /** Map: filenames -> file indexes */
        TokenFilesMap m_FilesMap; /** Map: file indexes -> Sets of TokenIndexes */
        TokenFilesSet m_FilesToBeReparsed; /** Set: file indexes */
        TokenIdxList m_FreeTokens; /** List of all the deleted (and available) tokens */

        /** List of tokens belonging to the global namespace */
        TokenIdxSet m_TopNameSpaces,
        m_GlobalNameSpace;

        TokenFilesStatus m_FilesStatus; /** Parse Status for each file */
        bool m_modified;
    protected:
        Token* GetTokenAt(int idx);
        int AddToken(Token* newToken,int forceidx = -1);

        void RemoveToken(int idx);
        void RemoveToken(Token* oldToken);

        int AddTokenToList(Token* newToken,int forceidx = -1);
        void RemoveTokenFromList(int idx);

        void RecalcFullInheritance(int parentIdx, TokenIdxSet& result); // called by RecalcData
};


inline void SaveIntToFile(wxOutputStream* f, int i)
{
    /* This used to be done as
        f->Write(&i, sizeof(int));
    which is incorrect because it assumes a consistant byte order
    and a constant int size */
/*
    unsigned int const j = i; // rshifts aren't well-defined for negatives
    wxChar c[4] = { j>>0&0xFF, j>>8&0xFF, j>>16&0xFF, j>>24&0xFF };
    f->Write( c, 4 );
*/
}

inline bool LoadIntFromFile(wxInputStream* f, int* i)
{
//    See SaveIntToFile
//    return f->Read(i, sizeof(int)) == sizeof(int);
/*
    wxChar c[4];
//vfc    if ( f->Read( c, 4 ).LastRead() != 4 ) return false;
	    if ( f->Read( c, 4 ) != 4 ) return false;
    *i = ( c[0]<<0 | c[1]<<8 | c[2]<<16 | c[3]<<24 );
*/
    return true;
}

inline void SaveStringToFile(wxOutputStream* f, const wxString& str)
{
/*
    const wxWX2MBbuf psz = str.mb_str(wxConvUTF8);
    // TODO (MortenMacFly#5#): Can we safely use strlen here?
    int size = psz ? strlen(psz) : 0;
    if (size >= 32767)
        size = 32767;
    SaveIntToFile(f, size);
    if(size)
        f->Write(psz, size);
*/
}

inline bool LoadStringFromFile(wxInputStream* f, wxString& str)
{
/*
    int size;
    if (!LoadIntFromFile(f, &size))
        return false;
    bool ok = true;
    if (size > 0 && size <= 32767)
    {
        wxChar buf[32768];
        ok = f->Read(buf, size).LastRead() == (size_t)size;
        buf[size] = '\0';
        str = wxString(buf, wxConvUTF8);
    }
    else // doesn't fit in our buffer, but still we have to skip it
    {
        str.Empty();
        size = size & 0xFFFFFF; // Can't get any longer than that
        f->SeekI(size, wxFromCurrent);
    }
    return ok;
*/
	return true;
}

#endif // TOKEN_H

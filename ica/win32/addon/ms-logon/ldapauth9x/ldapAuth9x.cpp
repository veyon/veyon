//ldapAuth9x.cpp
/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 UltraVNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com
// /macine-vnc Greg Wood (wood@agressiv.com)


#include "ldapauth9x.h"

/////////////////////////

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                                         )
{
    switch (ul_reason_for_call)
        {
                case DLL_PROCESS_ATTACH:
                case DLL_THREAD_ATTACH:
                case DLL_THREAD_DETACH:
                case DLL_PROCESS_DETACH:
                        break;
    }
    return TRUE;
}




LDAPAUTH9X_API
BOOL CUGP(char * userin,char *password,char *machine, char * groupin,int locdom)
{
        OSVERSIONINFO ovi = { sizeof ovi };
        GetVersionEx( &ovi );
        //if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
    //  ovi.dwMajorVersion >= 5 )
        if ( 5 >= 5 )
        {
        //Handle the command line arguments.
        LPOLESTR pszBuffer = new OLECHAR[MAX_PATH*2];
        LPOLESTR pszBuffer2 = new OLECHAR[MAX_PATH*2];
        LPOLESTR pszBuffer3 = new OLECHAR[MAX_PATH*2];
        LPOLESTR pszBuffer4 = new OLECHAR[MAX_PATH*2];
        mbstowcs( (wchar_t *) pszBuffer, userin, MAX_PATH );
        mbstowcs( (wchar_t *) pszBuffer2, password, MAX_PATH );
        mbstowcs( (wchar_t *) pszBuffer3, machine, MAX_PATH );
        mbstowcs( (wchar_t *) pszBuffer4, groupin, MAX_PATH );
        HRESULT hr = S_OK;
        //Get rootDSE and the domain container's DN.
        IADs *pObject = NULL;
        IADs *pObjectUser = NULL;
        IADs *pObjectGroup = NULL;
        IDirectorySearch *pDS = NULL;
        LPOLESTR szPath = new OLECHAR[MAX_PATH];
        LPOLESTR myPath = new OLECHAR[MAX_PATH];
        
        wcscpy(szPath,L"LDAP://dc1.ad.local/dc=ad,dc=local"); //set to root of domain or search path
        wprintf(szPath);
        wprintf(L"\n");
        //VariantClear(&var);
        if (pObject)
                {
                        pObject->Release();
                        pObject = NULL;
                }
        wprintf( szPath);
        wprintf(L"\n");
        //Bind to the root of the current domain.
        hr = ADsOpenObject(szPath,pszBuffer,pszBuffer2,
                         ADS_SECURE_AUTHENTICATION,IID_IDirectorySearch,(void**)&pDS);
        if (SUCCEEDED(hr))
                {
                        if (SUCCEEDED(hr))
                                {
                                        hr =  FindUserByName(pDS, pszBuffer, pszBuffer, pszBuffer2, &pObjectUser);
                                        if (FAILED(hr))
                                                {
                                                        wprintf(L"User not found %i\n",hr);
                                                        delete [] pszBuffer;
                                                        delete [] pszBuffer2;
                                                        delete [] pszBuffer3;
                                                        delete [] szPath;
                                                        delete [] myPath;
                                                        if (pDS) pDS->Release();
                                                        if (pObjectUser) pObjectUser->Release();
                                                        return false;
                                                }
                                        if (pObjectUser) pObjectUser->Release();
                                        ///////////////////// VNCACCESS
                                        hr =  FindGroup(pDS, pszBuffer, pszBuffer, pszBuffer2, &pObjectGroup,pszBuffer4);
                                        if (pObjectGroup)
                                                                {
                                                                        pObjectGroup->Release();
                                                                        pObjectGroup = NULL;
                                                                }
                                        if (FAILED(hr)) wprintf(L"group not found\n");
                                        if (SUCCEEDED(hr))
                                                {
                                                        wprintf(L"Group found OK\n");
                                                        IADsGroup *     pIADsG;
                                                        hr = ADsOpenObject( gbsGroup,pszBuffer, pszBuffer2, 
                                                                        ADS_SECURE_AUTHENTICATION,IID_IADsGroup, (void**) &pIADsG);
                                                        if (SUCCEEDED(hr))
                                                                {
                                                                        VARIANT_BOOL bMember = FALSE;  
                                                                        hr = pIADsG->IsMember(gbsMember,&bMember);
                                                                        if (SUCCEEDED(hr))
                                                                                {
                                                                                        if (bMember == -1)
                                                                                                {
                                                                                                        wprintf(L"Object \n\n%s\n\n IS a member of the following Group:\n\n%s\n\n",gbsMember,gbsGroup);
                                                                                                        delete [] pszBuffer;
                                                                                                        delete [] pszBuffer2;
                                                                                                        delete [] pszBuffer3;
                                                                                                        delete [] szPath;
                                                                                                        delete [] myPath;
                                                                                                        if (pDS) pDS->Release();
                                                                                                        return true;
                                                                                                }
                                                                                        else
                                                                                                {
                                                                                                        BSTR bsMemberGUID = NULL;
                                                                                                        IDirectoryObject * pDOMember = NULL;
                                                                                                        hr = ADsOpenObject( gbsMember,pszBuffer, pszBuffer2, 
                                                                                                                        ADS_SECURE_AUTHENTICATION,IID_IDirectoryObject, (void**) &pDOMember);
                                                                                                        if (SUCCEEDED(hr))
                                                                                                                {
                                                                                                                        hr = GetObjectGuid(pDOMember,bsMemberGUID);
                                                                                                                        pDOMember->Release();
                                                                                                                        pDOMember  = NULL;
                                                                                                                        if (RecursiveIsMember(pIADsG,bsMemberGUID,gbsMember,true, pszBuffer, pszBuffer2))
                                                                                                                                {
                                                                                                                                        delete [] pszBuffer;
                                                                                                                                        delete [] pszBuffer2;
                                                                                                                                        delete [] pszBuffer3;
                                                                                                                                        delete [] szPath;
                                                                                                                                        delete [] myPath;
                                                                                                                                        if (pDS) pDS->Release();
                                                                                                                                        return true;
                                                                                                                                }
                                                                                                                }
                                                                                        }//else bmember
                                                                        }//ismember
                                                        }//iadsgroup 
                                        }//Findgroup
                                        wprintf(L"USER not found in group\n");
                                        
                                }//user
                }
        if (pDS) pDS->Release();

                /*LOGFAILED(pszBuffer3,pszBuffer);*/
                delete [] pszBuffer;
                delete [] pszBuffer2;
                delete [] pszBuffer3;
                delete [] szPath;
                delete [] myPath;
                return false;
        }
        return false;
}

HRESULT FindUserByName(IDirectorySearch *pSearchBase, //Container to search
                                           LPOLESTR szFindUser, LPOLESTR  pwszUser, LPOLESTR pwszPassword, //Name of user to find.
                                           IADs **ppUser) //Return a pointer to the user
{
    HRESULT hrObj = E_FAIL;
    HRESULT hr = E_FAIL;
        if ((!pSearchBase)||(!szFindUser))
                return E_INVALIDARG;
        //Create search filter
        LPOLESTR pszSearchFilter = new OLECHAR[MAX_PATH];
        LPOLESTR szADsPath = new OLECHAR[MAX_PATH];
        wcscpy(pszSearchFilter, L"(&(objectClass=user)(samAccountName=");
        wcscat(pszSearchFilter, szFindUser);
        wcscat(pszSearchFilter, L"))");
    //Search entire subtree from root.
        ADS_SEARCHPREF_INFO SearchPrefs;
        SearchPrefs.dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
        SearchPrefs.vValue.dwType = ADSTYPE_INTEGER;
        SearchPrefs.vValue.Integer = ADS_SCOPE_SUBTREE;
    DWORD dwNumPrefs = 1;
        // COL for iterations
    ADS_SEARCH_COLUMN col;
    // Handle used for searching
    ADS_SEARCH_HANDLE hSearch;
        // Set the search preference
    hr = pSearchBase->SetSearchPreference( &SearchPrefs, dwNumPrefs);
    if (FAILED(hr))
        return hr;
        // Set attributes to return
        CONST DWORD dwAttrNameSize = 1;
    LPOLESTR pszAttribute[dwAttrNameSize] = {L"ADsPath"};

    // Execute the search
        wprintf(pszSearchFilter);
        wprintf(L"\n");
    hr = pSearchBase->ExecuteSearch(pszSearchFilter,
                                          pszAttribute,
                                                                  dwAttrNameSize,
                                                                  &hSearch
                                                                  );
        if (SUCCEEDED(hr))
        {    

    // Call IDirectorySearch::GetNextRow() to retrieve the next row 
    //of data
        while( pSearchBase->GetNextRow( hSearch) != S_ADS_NOMORE_ROWS )
                {
            // loop through the array of passed column names,
            // print the data for each column
            for (DWORD x = 0; x < dwAttrNameSize; x++)
            {
                            // Get the data for this column
                hr = pSearchBase->GetColumn( hSearch, pszAttribute[x], &col );
                            if ( SUCCEEDED(hr) )
                            {
                                    // Print the data for the column and free the column
                                        // Note the attribute we asked for is type CaseIgnoreString.
                    wcscpy(szADsPath, col.pADsValues->CaseIgnoreString); 
                                        hr = ADsOpenObject(szADsPath,
                                                                         pwszUser,
                                                                         pwszPassword,
                                                                         ADS_SECURE_AUTHENTICATION, //Use Secure Authentication
                                                                         IID_IADs,
                                                                         (void**)ppUser);
                                        if (SUCCEEDED(hr))
                                        { 
                       wprintf(L"%s: %s\r\n",pszAttribute[x],col.pADsValues->CaseIgnoreString); 
                                           hrObj = S_OK;
                                           gbsMember=SysAllocString(col.pADsValues->CaseIgnoreString);
                                        }
                                    pSearchBase->FreeColumn( &col );
                            }
                            else
                                    hr = E_FAIL;
            }
                }
                // Close the search handle to clean up
        pSearchBase->CloseSearchHandle(hSearch);
        }
        if (FAILED(hrObj))
                hr = hrObj;
    return hr;
}

HRESULT FindGroup(IDirectorySearch *pSearchBase, //Container to search
                                           LPOLESTR szFindUser, LPOLESTR  pwszUser, LPOLESTR pwszPassword, //Name of user to find.
                                           IADs **ppUser,LPOLESTR szGroup) //Return a pointer to the user
{
    HRESULT hrObj = E_FAIL;
    HRESULT hr = E_FAIL;
        if ((!pSearchBase)||(!szFindUser))
                return E_INVALIDARG;
        //Create search filter
        LPOLESTR pszSearchFilter = new OLECHAR[MAX_PATH];
        LPOLESTR szADsPath = new OLECHAR[MAX_PATH];
        wcscpy(pszSearchFilter, L"(&(objectClass=group)(cn=");
        wcscat(pszSearchFilter,szGroup);
        wcscat(pszSearchFilter, L"))");
    //Search entire subtree from root.
        ADS_SEARCHPREF_INFO SearchPrefs;
        SearchPrefs.dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
        SearchPrefs.vValue.dwType = ADSTYPE_INTEGER;
        SearchPrefs.vValue.Integer = ADS_SCOPE_SUBTREE;
    DWORD dwNumPrefs = 1;
        // COL for iterations
    ADS_SEARCH_COLUMN col;
    // Handle used for searching
    ADS_SEARCH_HANDLE hSearch;
        // Set the search preference
    hr = pSearchBase->SetSearchPreference( &SearchPrefs, dwNumPrefs);
    if (FAILED(hr))
        return hr;
        // Set attributes to return
        CONST DWORD dwAttrNameSize = 1;
    LPOLESTR pszAttribute[dwAttrNameSize] = {L"ADsPath"};

    // Execute the search
    hr = pSearchBase->ExecuteSearch(pszSearchFilter,
                                          pszAttribute,
                                                                  dwAttrNameSize,
                                                                  &hSearch
                                                                  );
        if (SUCCEEDED(hr))
        {    

    // Call IDirectorySearch::GetNextRow() to retrieve the next row 
    //of data
        while( pSearchBase->GetNextRow( hSearch) != S_ADS_NOMORE_ROWS )
                {
            // loop through the array of passed column names,
            // print the data for each column
            for (DWORD x = 0; x < dwAttrNameSize; x++)
            {
                            // Get the data for this column
                hr = pSearchBase->GetColumn( hSearch, pszAttribute[x], &col );
                            if ( SUCCEEDED(hr) )
                            {
                                    // Print the data for the column and free the column
                                        // Note the attribute we asked for is type CaseIgnoreString.
                    wcscpy(szADsPath, col.pADsValues->CaseIgnoreString); 
                                        hr = ADsOpenObject(szADsPath,
                                                                         pwszUser,
                                                                         pwszPassword,
                                                                         ADS_SECURE_AUTHENTICATION, //Use Secure Authentication
                                                                         IID_IADs,
                                                                         (void**)ppUser);
                                        if (SUCCEEDED(hr))
                                        { 
                       wprintf(L"%s: %s\r\n",pszAttribute[x],col.pADsValues->CaseIgnoreString); 
                                           hrObj = S_OK;
                                           gbsGroup=SysAllocString(col.pADsValues->CaseIgnoreString);
                                        }
                                    pSearchBase->FreeColumn( &col );
                            }
                            else
                                    hr = E_FAIL;
            }
                }
                // Close the search handle to clean up
        pSearchBase->CloseSearchHandle(hSearch);
        }
        if (FAILED(hrObj))
                hr = hrObj;
    return hr;
}




////////////////////////////////////////////////////////////////////////////////////////////////////
/*  RecursiveIsMember()                       - Recursively scans the members of passed IADsGroup ptr
                                                and any groups that belong to the passed ptr- for membership
                                                of Passed group.
                                                Will return a TRUE if the member is found in the passed group,
                                                or if the passed member is a member of any group which is a member
                                                of the passed group.
    Parameters
 
        IADsGroup *     pADsGroup       - Group from which to verify members.
        LPWSTR          pwszMember      - LDAP path for object to verify membership.
        BOOL            bVerbose        - IF TRUE, will output verbose information for the scan.
 
    OPTIONAL Parameters
 
       LPOLESTR  pwszUser           - User Name and Password, if the parameters are not passed, 
       LPOLESTER pwszPassword       - binding will use ADsGetObject; if the parameters are
                                    - specified, ADsOpenObject is used, passing user name and password.
*/
 
BOOL RecursiveIsMember(IADsGroup * pADsGroup,LPWSTR pwszMemberGUID,LPWSTR pwszMemberPath, 
                                             BOOL bVerbose, LPOLESTR  pwszUser, LPOLESTR pwszPassword)
{
    HRESULT         hr                = S_OK;     // COM Result Code
    IADsMembers *   pADsMembers       = NULL;     // Ptr to Members of the IADsGroup
    BOOL            fContinue         = TRUE;     // Looping Variable
    IEnumVARIANT *  pEnumVariant      = NULL;     // Ptr to the Enum variant
    IUnknown *      pUnknown          = NULL;     // IUnknown for getting the ENUM initially
    VARIANT         VariantArray[FETCH_NUM];      // Variant array for temp holding returned data
    ULONG           ulElementsFetched = NULL;     // Number of elements retrieved
    BSTR            bsGroupPath       = NULL;
    BOOL            bRet              = FALSE;

    if(!pADsGroup || !pwszMemberGUID || !pwszMemberPath)
    {
        return FALSE;
    }
 
    // Get the path of the object passed in
    hr = pADsGroup->get_ADsPath(&bsGroupPath);
 
    if (!SUCCEEDED(hr))
        return hr;
 
    if (bVerbose)
    {
        WCHAR pwszOutput[2048];
        wsprintf(pwszOutput,L"Checking the Group:\n\n%s\n\n for the member:\n\n%s\n\n",bsGroupPath,pwszMemberPath);
        PrintBanner(pwszOutput);
    }
 
    // Get an interface pointer to the IADsCollection of members
    hr = pADsGroup->Members(&pADsMembers);
 
    if (SUCCEEDED(hr))
    {
        // Query the IADsCollection of members for a new ENUM Interface
        // Be aware that the enum comes back as an IUnknown *
        hr = pADsMembers->get__NewEnum(&pUnknown);
 
        if (SUCCEEDED(hr))
        {
            // QI the IUnknown * for an IEnumVARIANT interface
            hr = pUnknown->QueryInterface(IID_IEnumVARIANT, (void **)&pEnumVariant);
 
            if (SUCCEEDED(hr))
            {
                // While have not hit errors or end of data....
                while (fContinue) 
                {
                   ulElementsFetched = 0;
                    // Get a "batch" number of group members-number of rows specified by FETCH_NUM
                    hr = ADsEnumerateNext(pEnumVariant, FETCH_NUM, VariantArray, &ulElementsFetched);
 
                    if (ulElementsFetched )
                    {
                        // Loop through the current batch-printing the path for each member.
                        for (ULONG i = 0; i < ulElementsFetched; i++ ) 
                        {
                            IDispatch * pDispatch         = NULL; // ptr for holding dispath of element
                            BSTR        bstrCurrentPath   = NULL; // Holds path of object
                            BSTR        bstrGuidCurrent   = NULL; // Holds path of object
                            IDirectoryObject * pIDOCurrent = NULL;// Holds the current object          
 
                            // Get the dispatch ptr for the variant
                            pDispatch = VariantArray[i].pdispVal;
//                            assert(HAS_BIT_STYLE(VariantArray[i].vt,VT_DISPATCH));
 
                            // Get the IADs interface for the "member" of this group
                            hr = pDispatch->QueryInterface(IID_IDirectoryObject,
                                                           (VOID **) &pIDOCurrent ) ;
 
                            if (SUCCEEDED(hr))
                            {
                                // Get the GUID for the current object
                                hr = GetObjectGuid(pIDOCurrent,bstrGuidCurrent);
 
                                if (FAILED(hr))
                                    return hr;
 
                                IADs * pIADsCurrent = NULL;
 
                                // Retrieve the IADs Interface for the current object
                                hr = pIDOCurrent->QueryInterface(IID_IADs,(void**)&pIADsCurrent);
                                if (FAILED(hr))
                                    return hr;
 
                                // Get the ADsPath property for this member
                                hr = pIADsCurrent->get_ADsPath(&bstrCurrentPath);
 
                                if (SUCCEEDED(hr))
                                {
                                    if (bVerbose)
                                        wprintf(L"Comparing:\n\n%s\nWITH:\n%s\n\n",bstrGuidCurrent,pwszMemberGUID);
                                    
                                    // Verify that the member of this group is Equal to passed.
                                    if (_wcsicmp(bstrGuidCurrent,pwszMemberGUID)==0)
                                    {
                                        if (bVerbose)
                                            wprintf(L"!!!!!Object:\n\n%s\n\nIs a member of\n\n%s\n\n",pwszMemberPath,bstrGuidCurrent);   
 
                                        bRet = TRUE;
                                        break;
                                    }
                                    else // Otherwise, bind to this and see if it is a group.
                                    {    // If is it a group then the QI to IADsGroup succeeds
                                        
                                        IADsGroup * pIADsGroupAsMember = NULL;
                                        
                                        if (pwszUser)
                                            hr = ADsOpenObject( bstrCurrentPath,
                                                                pwszUser, 
                                                                pwszPassword, 
                                                                ADS_SECURE_AUTHENTICATION,
                                                                IID_IADsGroup, 
                                                                (void**) &pIADsGroupAsMember);
                                        else
                                            hr = ADsGetObject( bstrCurrentPath, IID_IADsGroup,(void **)&pIADsGroupAsMember);
 
                                        // If bind was completed, then this is a group.
                                        if (SUCCEEDED(hr))
                                        {
                                            // Recursively call this group to verify this group.
                                            BOOL bRetRecurse;
                                            bRetRecurse = RecursiveIsMember(pIADsGroupAsMember,pwszMemberGUID,pwszMemberPath,bVerbose,pwszUser ,pwszPassword );
                                            
                                            if (bRetRecurse)
                                            {
                                                bRet = TRUE;
                                                break;
                                            }
                                            pIADsGroupAsMember->Release();
                                            pIADsGroupAsMember = NULL;
                                        }
                                    }
                                    SysFreeString(bstrCurrentPath);
                                    bstrCurrentPath = NULL;
 
                                    SysFreeString(bstrGuidCurrent);
                                    bstrGuidCurrent = NULL;
                                }
                                // Release
                                pIDOCurrent->Release();
                                pIDOCurrent = NULL;
                                if (pIADsCurrent)
                                {
                                    pIADsCurrent->Release();
                                    pIADsCurrent = NULL;
                                }
                            }
                         }
                        // Clear the variant array.
                        memset(VariantArray, 0, sizeof(VARIANT)*FETCH_NUM);
                    }
                    else
                        fContinue = FALSE;
                }
                pEnumVariant->Release();
                pEnumVariant = NULL;
            }
            pUnknown->Release();
            pUnknown = NULL;
        }
        pADsMembers ->Release();
        pADsMembers  = NULL;
    }
 
    // Free the group path if retrieved.
    if (bsGroupPath)
    {
        SysFreeString(bsGroupPath);
        bsGroupPath = NULL;
    }
    return bRet;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
/*  GetObjectGuid()    - Retrieves the GUID, in string form, from the passed Directory Object.
                         Returns S_OK on success.
 
    Parameters
 
        IDirectoryObject *  pDO     -   Directory Object from where GUID is retrieved.
        BSTR             &  bsGuid  -   Returned GUID            
*/
 
HRESULT GetObjectGuid(IDirectoryObject * pDO,BSTR &bsGuid)
{
 
    GUID *pObjectGUID   = NULL;
    PADS_ATTR_INFO      pAttributeEntries;
    LPWSTR              pAttributeName = L"objectGUID";  // Get the GUID for the object.
    DWORD               dwAttributesReturned = 0;
    HRESULT             hr;

    if(!pDO)
    {
        return E_FAIL;
    }

    hr = pDO->GetObjectAttributes(  &pAttributeName, // objectGUID
                                    1, //Only objectGUID
                                    &pAttributeEntries, // Returned attributes
                                    &dwAttributesReturned // Number of attributes returned
                                    );
 
    if (SUCCEEDED(hr) && dwAttributesReturned>0)
    {
        // Be sure the right type was retrieved--objectGUID is ADSTYPE_OCTET_STRING
        if (pAttributeEntries->dwADsType == ADSTYPE_OCTET_STRING)
        {
            // Get COM converted string version of the GUID
            // lpvalue should be LPBYTE. Should be able to cast it to ptr to GUID.
            pObjectGUID = (GUID*)(pAttributeEntries->pADsValues[0].OctetString.lpValue);
            
            // OLE str to fit a GUID
            
            LPOLESTR szGUID = new WCHAR [64];
            szGUID[0]=NULL;
            // Convert GUID to string.
            ::StringFromGUID2(*pObjectGUID, szGUID, 39); 
            bsGuid = SysAllocString(szGUID);
            
            delete [] szGUID;
         }
    }
 
    return hr;
 
}
////////////////////////////////////////////////////////////////////////////////////////////////////
/*  PrintBanner()   -       Prints a banner to the screen
 
    Parameters
 
                LPOLESTR pwszBanner   - String to print
*/
void PrintBanner(LPOLESTR pwszBanner)
{
    _putws(L"");
    _putws(L"////////////////////////////////////////////////////");
    wprintf(L"\t");
    _putws(pwszBanner);
    _putws(L"////////////////////////////////////////////////////\n");
}
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "ScraperOperations.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "FileItem.h"
#include "Util.h"
#include "utils/log.h"
#include "filesystem/AddonsDirectory.h"
#include "addons/AddonManager.h"
#include "addons/Scraper.h"
#include "video/VideoInfoTag.h"
#include "video/VideoInfoDownloader.h"
#include "Util.h"

using namespace JSONRPC;
using namespace ADDON;
using namespace XFILE;

JSON_STATUS CScraperOperations::GetScrapers(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CStdString type = parameterObject["type"].asString();
  type = type.ToLower();

  int scraperType;
  if (type.Equals("albums"))
    scraperType = ADDON_SCRAPER_ALBUMS;
  else if (type.Equals("artists"))
    scraperType = ADDON_SCRAPER_ARTISTS;
  else if (type.Equals("movies"))
    scraperType = ADDON_SCRAPER_MOVIES;
  else if (type.Equals("tvshows"))
    scraperType = ADDON_SCRAPER_TVSHOWS;
  else if (type.Equals("musicvideos"))
    scraperType = ADDON_SCRAPER_MUSICVIDEOS;
  else if (type.Equals("library"))
    scraperType = ADDON_SCRAPER_LIBRARY;
  else
    return OK;

  VECADDONS scrapers;
  if (CAddonMgr::Get().GetAddons((TYPE)scraperType, scrapers))
  {
    CFileItemList items;
    for (ADDON::IVECADDONS i = scrapers.begin(); i != scrapers.end(); ++i)
      items.Add(CAddonsDirectory::FileItemFromAddon(*i, ""));

    CVariant param = parameterObject["fields"];
    param["fields"] = CVariant(CVariant::VariantTypeArray);
    param["fields"].append("file");

    HandleFileItemList(NULL, true, "scrapers", items, param, result);
  }

  return OK;
}

JSON_STATUS CScraperOperations::FindMovie(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CStdString strFile = parameterObject["file"].asString();
  CStdString strScraper = parameterObject["scraperid"].asString();

  AddonPtr addon;
  if (CAddonMgr::Get().GetAddon(strScraper, addon))
  {
    ScraperPtr info = boost::dynamic_pointer_cast<CScraper>(addon);
    if (info && info->Content() == CONTENT_MOVIES)
    {
      MOVIELIST movielist;
      CVideoInfoDownloader scraper(info);

      CStdString strTitle = CUtil::GetTitleFromPath(strFile, false);
      if ( !scraper.FindMovie(strTitle, movielist) )
        return OK;

      CFileItemList items;
      for (unsigned int i = 0; i < (unsigned int)movielist.size(); i++)
        for (unsigned int n = 0; n < (unsigned int)movielist[i].m_url.size(); n++)
        {
          CFileItemPtr item = CFileItemPtr(new CFileItem( movielist[i].strTitle ));
          item->SetPath( movielist[i].m_url[n].m_url );
          items.Add( item );
        }

      CVariant param = parameterObject["fields"];
      param["fields"] = CVariant(CVariant::VariantTypeArray);
      param["fields"].append("file");

      HandleFileItemList(NULL, true, "results", items, param, result);

      return OK;
    }
  }

  return OK;
}

JSON_STATUS CScraperOperations::GetMovieDetails(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CStdString strScraper = parameterObject["scraperid"].asString();
  CStdString strUrl = parameterObject["url"].asString();

  AddonPtr addon;
  if (CAddonMgr::Get().GetAddon(strScraper, addon))
  {
    ScraperPtr info = boost::dynamic_pointer_cast<CScraper>(addon);
    if (info && info->Content() == CONTENT_MOVIES)
    {
      CVideoInfoTag movieDetails;
      CVideoInfoDownloader scraper(info);

      if ( !scraper.GetDetails(strUrl, movieDetails) )
        return OK;

      HandleFileItem(NULL, true, "moviedetails", CFileItemPtr(new CFileItem(movieDetails)), parameterObject, parameterObject["fields"], result, false);
      return OK;
    }
  }

  return OK;
}

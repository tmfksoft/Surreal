/*
 *  UltimateIRCd - an Internet Relay Chat Daemon, include/version.h
 *
 *  Copyright (C) 1990-2007 by the past and present ircd coders, and others.
 *  Refer to the documentation within doc/authors/ for full credits and copyrights.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id: version.h 986 2007-02-04 20:55:46Z shadowmaster $
 *
 */


#ifndef __versioninclude
#define __versioninclude 1

/*
 * UIRCd Protocol version revision
 */

#define UltimateProtocol 3000

/*
** Kill links with protocol older than this - ShadowMaster
*/
#define KillProtocol 2900

/*
 * Build status
 *
 * RELEASE - Stable Release Version
 * CURRENT - SVN Snapshot
 * ALPHA   - Alpha Release Version
 * BETA    - Beta Release Version
 * RC      - Release Candidate Version
 */
#define RELEASE_STATUS	"RELEASE"

/*
 * Major version number
 * Upped when large important portions of functionality
 * have been altered/upgraded compared to the last major
 * version.
 * Good example is the recode based on Bahamut where major
 * version was upped from 2.x to 3.x where the changes where so
 * dramatical and important that a major version update occured.
 */
#define MAJOR_RELEASE "0"

/*
 * Minor version number
 * Used to designate large important portions of functionality
 * when the revision version have added significant changes to the
 * IRCd over the last minor release, or where the protocol has
 * been altered in some way over the last branch.
 * Example would be changing the config file system, or adding
 * new commands, user or channel modes.
 * Even numbers designate a stable branch, odd numbers
 * designate a development branch.
 */
#define MINOR_RELEASE "0.1"

/*
 * Revision version number
 * Used to designate bugfix and functionality releases that do not
 * break backwards compactibility with previous MINOR_RELEASE versions
 * unless the MINOR_RELEASE is an odd number.
 */
#define REVISION "01"

/*
 * Minor version branch name
 */
#define RELEASE_NAME "Breakaway"

/*
 Deliberate empty lines
*/

#define PATCH1  	\
			\
			\
			\
			""

/*
 Deliberate empty lines
*/

#define PATCH2  	\
			\
			\
			\
			""

/*
 Deliberate empty lines
*/

#define PATCH3  	\
			\
			\
			\
			""

/*
 Deliberate empty lines
*/

#define PATCH4  	\
			\
			\
			\
			""

/*
 Deliberate empty lines
*/

#define PATCH5  	\
			\
			\
			\
			""

/*
 Deliberate empty lines
*/

#define PATCH6  	\
			\
			\
			\
			""

/*
 Deliberate empty lines
*/

#define PATCH7  	\
			\
			\
			\
			""

/*
 Deliberate empty lines
*/

#define PATCH8  	\
			\
			\
			\
			""

/*
 Deliberate empty lines
*/

#define PATCH9 		\
			\
			\
			\
			""


#define BASE_VERSION "SurrealIRCD"

#define PATCHES PATCH1 PATCH2 PATCH3 PATCH4 PATCH5 PATCH6 PATCH7 PATCH8 PATCH9

void build_version (void);

#endif /* __versioninclude */

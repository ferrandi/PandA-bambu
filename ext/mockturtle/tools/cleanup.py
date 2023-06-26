#!/usr/bin/env python3
import fileinput
import os
import re
import subprocess

copyright_header = """/\* mockturtle: C\+\+ logic network library
 \* Copyright \(C\) ([\d-]+)  EPFL
 \*
 \* Permission is hereby granted, free of charge, to any person
 \* obtaining a copy of this software and associated documentation
 \* files \(the \"Software\"\), to deal in the Software without
 \* restriction, including without limitation the rights to use,
 \* copy, modify, merge, publish, distribute, sublicense, and/or sell
 \* copies of the Software, and to permit persons to whom the
 \* Software is furnished to do so, subject to the following
 \* conditions:
 \*
 \* The above copyright notice and this permission notice shall be
 \* included in all copies or substantial portions of the Software.
 \*
 \* THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,
 \* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 \* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 \* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 \* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 \* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 \* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 \* OTHER DEALINGS IN THE SOFTWARE.
 \*/
"""

copyright_header_replace = """/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2022  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
"""

file_header_match = """/\*\!
\s+\\\\file\s+([\w.]+)
\s+\\\\brief\s+([^\n\\\\]*)
(\s+\\\\author[^\*]*)\*/"""

file_header_replace = """/*!
  \\file {}
  \\brief {}

{}
*/"""


def find_files( path, ext ):
    items = []
    for root, dirs, files in os.walk( path ):
        for name in files:
            if name.endswith( ext ):
                items.append( os.path.join( root, name ) )
    return items

def read_file( filename ):
    with open( filename, "r" ) as f:
        content = f.read().splitlines()
    return content

def write_file( filename, content ):
    with open( filename, "w" ) as f:
        f.write( '\n'.join( content ) )

def match_replace( content, pattern, replace ):
    found = False
    for match in pattern.finditer( content ):
        found = True
        content = ''.join( [content[:match.start()], replace, content[match.end():]] )
    return found, content

def match_file_header( content, pattern, filename, authors ):
    found = False
    for match in pattern.finditer( content ):
        found = True

        if match[1] != filename:
            print( "[w] filename does not match {} {}".format( match[1], filename ) )

        replace = file_header_replace.format( filename, match[2], "".join( ["  \\author {}\n".format( a ) for a in authors] ).rstrip() )
        content = ''.join( [content[:match.start()], replace, content[match.end():]] )

    return found, content

author_pseudonyms = {
    'Eleonora' : 'Eleonora Testa',
    'b1f6c1c4' : 'Jinzheng Tu',
    'eletesta' : 'Eleonora Testa',
    'gmeuli' : 'Giulia Meuli',
    'lee30sonia' : 'Siang-Yun (Sonia) Lee',
    'mdsudara' : 'Dewmini Sudara',
    'wlneto' : 'Walter Lau Neto',
    'zfchu' : 'Zhufei Chu',
    'Andrea' : 'Andrea Costamagna'
}
def git_authors( file ):
    result = subprocess.check_output(['git', 'shortlog', '-s', '--', file])

    authors = []
    lines = result.decode('utf-8').split('\n')
    for l in lines:
        author_information = l.split( '\t' )
        if len( author_information ) > 1:
            name = author_information[1]
            if ( name in author_pseudonyms ):
                authors.append( author_pseudonyms[name] )
            else:
                authors.append( name )

    # remove duplicate names
    authors = list( dict.fromkeys( authors ) )
    return sorted( authors )

# find all `.hpp` files
hpp_files = find_files( 'include/', '.hpp' )
cpp_files = find_files( 'experiments/', '.cpp' )

# prepare regular expression patterns
copyright_header_pattern = re.compile( copyright_header )
file_header_pattern = re.compile( file_header_match )

for path in hpp_files:
    filename = os.path.basename( path )

    # determine authors of file
    authors = git_authors( path )

    content = read_file( path )
    content = '\n'.join( content )
    new_content = content

    # update copyright header if necessary
    copyright_header_found, new_content = match_replace( new_content, copyright_header_pattern, copyright_header_replace )

    # add copyright header if not found
    if not copyright_header_found:
        new_content = ''.join( [ copyright_header_replace, new_content ] )

    # update file header if necessary
    found, new_content = match_file_header( new_content, file_header_pattern, filename, authors )

    # rewrite the file
    if ( content != new_content ):
        print( "create backup ", path + "~" )
        write_file( path + "~", [ content ] )

        print( "update file ", path )
        write_file( path, [ new_content ] )

for path in cpp_files:
    filename = os.path.basename( path )

    # determine authors of file
    authors = git_authors( path )

    content = read_file( path )
    content = '\n'.join( content )
    new_content = content

    # update copyright header if necessary
    copyright_header_found, new_content = match_replace( new_content, copyright_header_pattern, copyright_header_replace )

    # add copyright header if not found
    if not copyright_header_found:
        new_content = ''.join( [ copyright_header_replace, new_content ] )

    # rewrite the file
    if ( content != new_content ):
        print( "create backup ", path + "~" )
        write_file( path + "~", [ content ] )

        print( "update file ", path )
        write_file( path, [ new_content ] )

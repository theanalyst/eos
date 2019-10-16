.. highlight:: rst

.. _tokens:

Using Eos Tokens forauthorization
=================================

To prepare for upcoming requirements to implement various types of token technologies, we have implemeneted a generic EOS mechanism to delegate permissions to bearer with s.c. EOS tokens.

The JSON format of an EOS token looks like shown here:

.. code-block:: bash

   {
    "token": {
    "permission": "rwx",
    "expires": "1571147001",
    "owner": "",
    "group": "",
    "generation": "0",
    "path": "/eos/dev/token/",
    "allowtree": true,
    "origins": []
   },
   "signature": "DFjL+dhAA3F6OZtntJpLgGwr1RibtHvaIy6+qKWeuXc=",
   "serialized": "CgNyd3gQ+ZmX7QUyFy9lb3MvYWpwL3Rva2VuL3RfdG9rZW4vOAE=",
   "seed": 201665152
  }

Essentially this token gives the bearer the permission to ``rwx`` under the tree /eos/dev/token/. The token does not bear an
owner and group information, which means, that the creations will be accounted on the mapped authenticated user using this token or an enforced ``sys.owner.auth`` entry. If the token should map the authenticated user, one can add ``owner`` and ``group`` fields. In practical terms the token removes existing user and system ACL entries and places the token user/group/permission entries as a system ACL.

Tokens are encrypted, zlib compressed, base64 encoded with a replacement of all '/' characters with '!' to avoid confusion with directory and file names.
   
Token creation
--------------

The CLI interface to create a token is shown here:

.. code-block:: bash

   # create a generic read-only token for a file valid 5 minutes
   EXPIRE=`date +%s; let LATER=$EXPIRE+300

   eos token --path /eos/myfile --expires $LATER
   zteos64:MDAwMDAwNzR4nONS4WIuKq8Q+Dlz+ltWI3H91Pxi~cSsAv2S~OzUPP2SeAgtpMAY7f1e31Ts+od+rgcLZ~a2~bhwcZO9cracyhm1b3c6jpRIEWWOws71Ox6xAABeTC8I

   # create a generic read-only token for a directory - mydir has to end with a '/' - valid 5 minutes
   eos token --path /eos/mydir/ --expires $LATER

   # create a generic read-only token for a directory tree - mytree has to end with a '/' - valid 5 minutes
   eos token --path /eos/mydir/ --tree --expires $LATER

   # create a generic write token for a file - valid 5 minutes
   eos token --path /eos/myfile --permission rwx --expires $LATER

Token inspection
----------------

The CLI interface to show the contents of a token is shown here:

.. code-block:: bash

   eos token --token zteos64:MDAwMDAwNzR4nONS4WIuKq8Q+Dlz+ltWI3H91Pxi~cSsAv2S~OzUPP2SeAgtpMAY7f1e31Ts+od+rgcLZ~a2~bhwcZO9cracyhm1b3c6jpRIEWWOws7

   TOKEN="zteos64:MDAwMDAwNzR4nONS4WIuKq8Q+Dlz+ltWI3H91Pxi~cSsAv2S~OzUPP2SeAgtpMAY7f1e31Ts+od+rgcLZ~a2~bhwcZO9cracy"
   
   env EOSAUTHZ=$TOKEN eos whoami
   Virtual Identity: uid=0 (99,3,0) gid=0 (99,4,0) [authz:unix] sudo* host=localhost domain=localdomain geo-location=ajp
   {
    "token": {
     "permission": "rx",
     "expires": "1600000000",
     "owner": "",
     "group": "",
     "generation": "0",
     "path": "/eos/myfile",
     "allowtree": false,
     "origins": []
    },
   }

Token usage
-----------

A file token can be used in two ways:

* as a filename
* via CGI '?authz=$TOKEN'

.. code-block:: bash

   # as a filename
   xrdcp root://myeos//zteos64:MDAwMDAwNzR4nONS4WIuKq8Q+Dlz+ltWI3H91Pxi~cSsAv2S~OzUPP2SeAgtpMAY7f1e31Ts+od+rgcLZ~a2~bhwcZO9cracy /tmp/

   # via CGI
   xrdcp "root://myeos//eos/myfile?authz=zteos64:MDAwMDAwNzR4nONS4WIuKq8Q+Dlz+ltWI3H91Pxi~cSsAv2S~OzUPP2SeAgtpMAY7f1e31Ts+od+rgcLZ~a2~bhwcZO9cracy" /tmp/

If a token contains a subtree permission, the only way to use it for a file access is to use the CGI form. The filename form is practical to hide the filename for up-/downloads.

Token issuing permission
------------------------

The ``root`` user can issue any token. Everybody else can only issue tokens for files in existing parent directories or directory trees, where the calling user is the current owner.

Token lifetime 
---------------

The token lifetime is given as a unix timestamp during the token creation. 

Token Revocation
----------------

Tokens are issued with a generation entry. The generation value is a globally configured incrementing number. In case of emergency all tokens can be revoked by increasing the generation value. Currently there is no CLI to modify the generation entry.

Token Origin Restrictions
-------------------------

The client location from where a token can be used can be restricted by using the ``origins`` entries.

.. code-block:: bash

   # all machines at CERN authenticating via kerberos as user nobody		
   eos token --path /eos/myfile --origin \*.cern.ch:nobody:krb5"

   # all machines at CERN authenticating via unix as user kubernetes from machine k8s.cern.ch
   eos token --path /eos/myfile --origin "k8s.cern.ch:kubernetes:unix"

   # general syntax is a regexp for origin like <regexp hostname>:<regexp username>:<regexp auth protocol>

The default origin regexp is ``*:*:*`` accepting all origins.





/*
 * Copyright 1995, 1996 Perforce Software.  All rights reserved.
 *
 * This file is part of Perforce - the FAST SCM System.
 */

/*
 * Options::Parse() - parse command line options
 *
 *	The "opts" string list flags.  Each (single character) flag x
 *	can be followed by an optional modifier:
 *
 *		x.	- flag takes an argument (-xarg)
 *		x:	- flag takes an argument (-xarg or -x arg)
 *		x?	- flag takes an optional argument (--long=arg only)
 *		x+	- flag takes a flag and arg (-xyarg or -xy arg)
 *		x#	- flag takes a non-neg numeric arg (-xN or -x N)
 *		x$	- same as : except it indicates to stop parsing after
 *		          if more arguments follow, only the first
 *		          will be assigned to option, the rest not parsed.
 */

const int N_OPTS = 256;

enum OptFlag {
	// Bitwise selectors

	OPT_ONE = 0x01,	        // exactly one
	OPT_TWO = 0x02,	        // exactly two
	OPT_THREE = 0x04,	// exactly three
	OPT_MORE = 0x08,	// more than two
	OPT_NONE = 0x10,	// require none
	OPT_MAKEONE = 0x20,	// if none, make one that points to null

	// combos of the above

	OPT_OPT = 0x11,	        // NONE, or ONE
	OPT_ANY = 0x1F,	        // ONE, TWO, THREE, MORE, or NONE
	OPT_DEFAULT = 0x2F,	// ONE, TWO, THREE, MORE, or MAKEONE
	OPT_SOME = 0x0F	        // ONE, TWO, THREE, or MORE
} ;

struct ErrorId;

class Options
{
    public:
			Options() { optc = 0; }
			Options( const Options &other );
	void		Reset() { optc = 0; }

	enum Opt {

	        // options which are used commonly, across many commands:

	                All            = 'a',
	                Archive        = 'A',
	                Change         = 'c',
	                Delete         = 'd',
	                Depot          = 'D',
	                Expression     = 'e',
	                NoCaseExpr     = 'E',
	                Force          = 'f',
	                Filter         = 'F',
	                Input          = 'i',
	                JournalPrefix  = 'J',
	                Long           = 'l',
	                Max            = 'm',
	                Preview        = 'n',
	                Output         = 'o',
	                OutputFlags    = 'O',
	                Port           = 'p',
	                Parent         = 'P',
	                Quiet          = 'q',
	                Reverse        = 'r',
	                Short          = 's',
	                Stream         = 'S',
	                Filetype       = 't',
	                Tags           = 'T',
	                User           = 'u',
	                Variable       = 'v',
	                Wipe           = 'w',
	                Compress       = 'z',

	        // options which are relatively uncommon, but have existing
	        // short-form versions:

	                InfrequentShortFormOptions = 1000,

	                Version        , // -V
	                Client         , // -c client
	                Shelf          , // -s shelf
	                DiffFlags      , // -d<diff-flags>
	                Inherited      , // -i on changes, filelog, etc.
	                ClientName     , // -C client
	                Charset        , // p4 -C charset
	                CmdCharset     , // p4 -Q charset
	                Help           , // -h for main programs
	                Batchsize      , // -b N
	                MessageType    , // 'p4 -s'
	                Xargs          , // 'p4 -x file'
	                Exclusive      , // opened -x
	                Directory      , // p4 -d dir
	                Host           , // p4 -H host
	                Password       , // -P password
	                Retries        , // p4 -r retries
	                Progress       , // p4 -I
	                NoIgnore       , // add -I
	                Downgrade      , // add -d
	                Unload         , // -U for unloaded objects
	                UnloadLimit    , // backup -u #
	                CentralUsers   , // users -c
	                ReplicaUsers   , // users -r
	                Branch         , // unshelve -b
	                FullBranch     , // branch -F
	                SpecFixStatus  , // change -s, submit -s
	                ChangeType     , // change -t
	                ChangeUpdate   , // change -u
	                Original       , // change -O, describe -O
	                ChangeUser     , // change -U
	                Template       , // client -t, label -t
	                Switch         , // client -s
	                Temporary      , // client -x
	                Owner          , // group -a
	                Administrator  , // group -A
	                Global         , // label/labelsync/tag -g
	                GlobalLock     , // lock/opened -g
	                StreamType     , // stream -t
	                VirtualStream  , // stream -v
	                Brief          , // changes -L, filelog -L
	                ShowTime       , // changes -t, filelog -t
	                ChangeStatus   , // changes -s
	                Exists         , // files -e
	                Blocksize      , // sizes -b
	                Shelved        , // sizes -S, describe -S
	                Summary        , // sizes -s
	                OmitLazy       , // sizes -z
	                Human1024      , // sizes -h
	                Human1000      , // sizes -H
	                LimitClient    , // list -C
	                LabelName      , // list -l
	                RunOnMaster    , // list -M
	                LeaveKeywords  , // print -k
	                LeaveKeywords2 , // sync -K, revert -K, integ -K...
	                OutputFile     , // print -o
	                Content        , // filelog -h
	                OmitPromoted   , // filelog -p
	                OmitMoved      , // filelog -1
	                KeepClient     , // edit -k, delete -k, sync -k
	                FileCharset    , // add/edit -Q
	                Virtual        , // delete -v
	                Generate       , // server -g
	                Configure      , // server -c
	                Usage          , // license -u
	                Job            , // fixes -j
	                Increment      , // counter -i
	                FixStatus      , // fix -s
	                Replace        , // shelve -r
	                ShelveOpts     , // shelve -a
	                SubmitShelf    , // submit -e
	                SubmitOpts     , // submit -f
	                Reopen         , // submit -r
	                Description    , // submit -d
	                Tamper         , // submit -t
	                BackgroundXfer , // submit -b
	                Date           , // unload -d
	                StreamName     , // unload -s, reload -s
	                Unchanged      , // revert -a
	                KeepHead       , // archive -h
	                Purge          , // archive -p
	                ForceText      , // archive -t
	                BinaryAsText   , // -t on annotate, diff, diff2,
	                                 // grep, resolve, merge3, ...
	                BypassFlow     , // -F on copy, interchanges, merge
	                ShowChange     , // annotate -c
	                FollowBranch   , // annotate -i
	                FollowInteg    , // annotate -I
	                SourceFile     , // -s on copy, integrate, merge,
	                                 // interchanges, populate
	                ResolveFlags   , // resolve -A<flags>
	                AcceptFlags    , // resolve -a<flags>
	                IntegFlags     , // integrate -R<flags>
	                DeleteFlags    , // integrate -D<flags>
	                RestrictFlags  , // fstat -R<flags>
	                SortFlags      , // fstat -S<flags>
	                ForceFlag      , // client -d -f -F<flags>
	                UseList        , // sync -L, fstat -L
	                Safe           , // sync -s
	                Publish        , // sync -p
	                ForceDelete    , // group -F/user -F
	                IsGroup        , // groups -g
	                IsUser         , // groups -u
	                IsOwner        , // groups -o
	                Verbose        , // groups -v
	                LineNumber     , // grep -n
	                InvertMatch    , // grep -v
	                FilesWithMatches,// grep -l
	                FilesWithoutMatch,// grep -L
	                NoMessages     , // grep -s
	                FixedStrings   , // grep -F
	                BasicRegexp    , // grep -G
	                ExtendedRegexp , // grep -E
	                PerlRegexp     , // grep -P
	                Regexp         , // grep -e
	                AfterContext   , // grep -A
	                BeforeContext  , // grep -B
	                Context        , // grep -C
	                IgnoreCase     , // grep -i
	                Repeat         , // pull -i
	                Backoff        , // pull -b
	                ArchiveData    , // pull -u
	                Status         , // pull -l
	                LocalJournal   , // pull -L
	                JournalPosition, // pull -j
	                PullServerid   , // pull -P
	                ExcludeTables  , // pull -T
	                File           , // pull -f
	                Revision       , // pull -r
	                Append         , // logappend -a, property -a
	                Sequence       , // logger -c
	                Counter        , // logger -t
	                HostName       , // login -h
	                Print          , // login -p
	                LoginStatus    , // login -s
	                StartPosition  , // logparse -s, logtail -s
	                Encoded        , // logparse -e
	                LogName        , // logtail -l
	                CompressCkp    , // p4 admin checkpoint -Z
	                SpecType       , // p4 admin updatespecdepot -s
	                MaxAccess      , // p4 protects -m
	                GroupName      , // p4 protects -g
	                ShowFiles      , // p4 interchanges -f
	                Name           , // attribute -n, property -n
	                Value          , // attribute -v, property -v
	                Propagating    , // attribute -p
	                Storage        , // attribute -T
	                OpenAdd        , // reconcile -a
	                OpenEdit       , // reconcile -e
	                OpenDelete     , // reconcile -d
	                OpenType       , // reconcile -t
	                UseModTime     , // reconcile -m
	                Local          , // reconcile -l
	                OutputBase     , // resolved -o
	                System         , // set -s
	                Service        , // set -S
	                Histogram      , // dbstat -h
	                TableNotUnlocked,// dbverify -U
	                TableName      , // dbverify -t
	                AllClients     , // lockstat -C
	                CheckSize      , // verify -s
	                Transfer       , // verify -t
	                Update         , // verify -u
	                Verify         , // verify -v
	                NoArchive      , // verify -X
	                Serverid       , // clients -s, labels -s
	                Unified        , // diff2 -u
	                PreviewNC      , // resolve -N
	                Estimates      , // sync -N/flush -N/update -N
	                Locked         , // unload -L
	                UnloadAll      , // unload -a
	                KeepHave       , // integrate -h
	                Yes            , // trust -y
	                No             , // trust -n
	                InputValue     , // trust -i
	                Replacement    , // trust -r
	                Rebuild        , // jobs -R
	                Equal          , // fstat -e
	                AttrPattern    , // fstat -A
	                DiffListFlag   , // diff -s
	                Arguments      , // monitor show -a
	                Environment    , // monitor show -e
	                TaskStatus     , // monitor show -s
	                AllUsers       , // property -A
	                Promote        , // shelve -p
	                Test           , // ldap -t, ldaps -t
	                Active         , // ldaps -A
	                GroupMode      , // ldapsync -g
	                UserMode       , // ldapsync -u
	                UserModeCreate , // ldapsync -u -c
	                UserModeCreateStrict, // ldapsync -u -C
	                UserModeUpdate , // ldapsync -u -U
	                UserModeDelete , // ldapsync -u -d
	                Create         , // switch -c
	                List           , // switch -l
	                Mainline       , // switch -m
	                MoveChanges    , // switch -r
	                ReplicationStatus, // servers --replication-status, servers -J
	                DepotType      , // depot -t
	                Users          , // annotate -u
	                Tab            , // annotate -T
	                Rename         , // move -r
	                DoAdded        , // describe -a 
	                Retry          , // pull -R
	                StorageType    , // --type
	                ByUser         , // --user username
	                ByOwner        , // --owner username
	                RepoName2      , // --repo -n
	                Depot2         , // --depot -d
	                Reference      , // --reference -r
	                Perm           , // --permission -p
	                ForceFailover  , // failover -F
	                IgnoreMaster   , // failover -i
	                RequireMaster  , // failover -m
	                FailbackYes    , // failback -y
	                FailbackQuiesce, // failback -w
	                FailoverYes    , // failover -y
	                Failoverid     , // failover -s
	                FailoverQuiesce, // failover -w
	                FailoverVerification, // failover -v
	                Install        , // --install (extension)
	                ChangeStart    , // integrated -s change
	                Target         , // heartbeat -t
	                Interval       , // heartbeat -i
	                Wait           , // heartbeat -w
	                MissingInterval, // heartbeat -m
	                MissingWait    , // heartbeat -r
	                MissingCount   , // heartbeat -c
	                LocalLicense   , // license -u -l
	                AutoReload     , // labels -R
	                IntervalMillis , // --interval-ms | -I
	                Threshold      , // topology -t
	                DatedEarlier   , // topology -e
	                DeleteMarker   , // topology -d
	                DeletePurge    , // topology -D
	                MoveTopology   , // topology -m
	                ServerAddress  , // topology -s
	                ServerID       , // topology -i
	                TargetAddress  , // topology -p
	                NewServerAddress, // topology -S
	                NewServerID    , // topology -I
	                NewTargetAddress, // topology -P
	                CreationDate   , // topology -c
	                LastSeenDate   , // topology -l
	                ListAddresses  , // license -L

	        // options which have only long-form option names go here:

	                LongFormOnlyOptions = 2000,

	                NoRejournal    , // pull --no-rejournal
	                From           , // renameuser or renameclient --from
	                To             , // renameuser or renameclient --to
	                Parallel       , // sync --parallel
	                ParallelSubmit , // submit --parallel
	                InputFile      , // reload --input-file
	                PidFile        , // p4d --pid-file
	                NoRetransfer   , // submit --noretransfer
	                ForceNoRetransfer, // submit --forcenoretransfer
	                DurableOnly    , // journalcopy --durable-only
	                NonAcknowledging, // journalcopy --non-acknowledging
	                BypassExclusiveLock, // open --bypass-exclusive-lock
	                RetainLbrRevisions, // unzip --retain-lbr-revisions
	                JavaProtocol   , // p4d -i --java
	                PullBatch      , // pull -u --batch=N
	                EnableDVCSTriggers, // unzip --enable-dvcs-triggers
	                ConvertAdminComments, // --convert-p4admin-comments
	                RemoteSpec     , // --remote
	                P4UserUser     , // --me
	                Aliases        , // --aliases
	                Field          , // --field
	                AtomicPush     , // receive-pack --atomic
	                ClientType     , // clients --client-type=graph
	                Color          , // --color
	                ChangeFiles    , // changes --show-files
	                DiscardArchives, // graph recieve-pack --discard-archives=N
	                LicenseInfo    , // p4d --license-info
	                RemoteUser     , // push/fetch/login --remote-user=X
	                IgnoreCMap     , // files/fstat/diff2 --ignore-changeview
	                Mirror         , // graph receive-pack --mirror

	                DaemonSafe     , // daemon with stdio closed
	                Trigger        , // pull -u --trigger
	                IgnoreHave     , // -p --ignore-have
	                GraphOnly      , // --graph-only
	                NoGraph        , // --no-graph
			MinSize        , // --min-size
			MaxSize        , // --max-size
			NameOnly       , // --name-only
			NoFastForward  , // --no-ff
			FastForwardOnly, // --ff-only
			MustExist      , // --exists
			RepoName       , // --repo (for merge)
			TargetBranch   , // --target branch (for merge)
			Squash         , // --squash
			AllowEmpty     , // --allow-empty
			CreateIndex    , // --create-index (on a repo)
			DropIndex      , // --drop-index (from a repo)
			FirstParent    , // --first-parent (filelog)
			Index          , // --index (filelog)
			Graph          , // --graph (filelog)
			Oneline        , // --oneline (filelog)
			NoAbbrev       , // --no-abbrev (filelog)
			OneParent      , // --one-parent (filelog)
			Merges         , // --merges (filelog)
			CreateSampleExtension, // --sample (extension)
			Undo           , // --undo (cherry-pick)
			ParentNumber   , // --parent-number (undo)
			PkgExtension   , // --package (extension)
			Script         , // --script
			ScriptMaxMem   , // --script-MaxMem
			ScriptMaxTime  , // --script-MaxTime
			ScriptEnableDbg, // --script-enable-debug
			Path           , // --path (extension)
			NoSync         , // --no-sync
			NoScript       , // --no-script
			ScriptLang     , // --script-lang
			ScriptLangVersion, // --script-lang-version
			IntoOnly       , // --into-only (integrated)
			ScriptAPIVersion, // --script-api-version
			RunExtension   , // --run (extension)
			ShowMemInfo    , // --show-mem-info
			Repair         , // --repair
			DeleteItem     , // --delete <item>
			Sign           , // --sign <dir> (extension)
			Cert           , // --cert (ext certificate)
			Comment        , // --comment comment
			AllowUnsigned  , // --allow-unsigned
			SSInherit      , // --inherit
			SSNoInherit    , // --noinherit
			SSSourceComments, // --source-comments
			SSParentView,    // --parentview
			SwitchStreamUnrelated  , // switch --allow-unrelated
			Only            , // --only BAD | MISSING
			ShowRealtime    , // --show-realtime
			CleanPurge      , // --purged-only
			ViewMatch       , // --viewmatch
			Obliterate      , // stream --obliterate
			Offset          , // print --offset
			Size            , // print --size
			Compressed      , // verify --compressed=0/1
			PreFailback     , // p4d --pre-failback
			PostFailback    , // p4d --post-failback
			StreamViews     , // --streamviews
			UseStreamChange	, // --use-stream-change
			HasStream       , // --stream
			NoStream        , // --nostream
			PreserveChangeNumbers, // --preserve-change-numbers
			Limit           , // --limit #
			Type            , // --type checkpoint|dump....
			Result          , // --result pass|fail
			JNum            , // --num #
			JField          , // --jfield f1,f2,f3
			ControlTweaks   , // --control-tweaks
			CachePurge      , // --cache-purge (p4p)
			Iteration       , // --iteration
#ifdef _DEBUG
			DebugBreak,    // --debugbreak
#endif

	                UnusedLastOption
	} ;

	void		Parse( int &argc, char **&argv, const char *opts, 
			       int flag, const ErrorId &usage, Error *e );

	void		ParseLong( int &argc, char **&argv, const char *opts, 
			           const int *longOpts,
			           int flag, const ErrorId &usage, Error *e );

	void		Parse( int &argc, StrPtr *&argv, const char *opts, 
			       int flag, const ErrorId &usage, Error *e );

	void		ParseLong( int &argc, StrPtr *&argv, const char *opts, 
			           const int *longOpts,
			           int flag, const ErrorId &usage, Error *e );

	void		ParseTest( int &argc, StrPtr *&argv, const char *opts, 
			           const int *longOpts, Error *e );

	StrPtr *	operator [](int opt) 
			{ return GetValue( opt, 0, 0 ); }

	StrPtr *	GetValue( int opt, int subopt )
			{ return GetValue( opt, 0, subopt ); }

	StrPtr *	GetValue( int opt, char flag2, int subopt );

	int		FormatOption( int i, Error *e );
	int		FormatOption( int i, StrBuf & f) const;
	int		HasOption( int i );
	void		GetOptionName( int i, StrBuf &sb );
	void		GetOptionValue( int i, StrBuf &sb );

	static int		FindCode( const int code, Error *e );
	static int		GetShortForm( const int ilist, Error *e );
	static const char *	GetLongForm( const int ilist, Error *e );
	void		Discard( int opt, char flag2 = 0, int subopt = 0 );
	void		Dump( StrPtr * );

    private:
	int 		optc;

	int		flags[ N_OPTS ];
	char		flags2[ N_OPTS ];
	StrRef		vals[ N_OPTS ];

	static struct OptionInfo {
	    const char *name;
	    int   optionCode;
	    int   shortForm;
	    int   valueType;
	    const ErrorId *help;
	} list[];
} ;


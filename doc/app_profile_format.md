# Libglvnd Application Profile Format

When libglvnd has to select a vendor for a screen, it will start by checking
for an application profile.

An profile is effectively just a list of vendor names that libglvnd should try.
Libglvnd will load each vendor listed in the profile and call into it to check
if the vendor supports each screen.

## Configuration Files

Libglvnd will look for configuration files with the suffix ".profile.json" from these
directories, in order:
* `/etc/glvnd/profiles.d/`
* `/usr/share/glvnd/profiles.d/`
* `/usr/share/glvnd/profiles.d/`

Each config file looks like this:

```JSON
{
	"version" : [ 0, 0 ],
	"profiles" : [
		{
			"match" : [ "executable_name" ],
			"override" : true,
			"vendors" : [
				{
					"vendor_name" : "mesa",
					"vendor_data" : { "vendor-private-data" : 12345 }
				}
			]
		}
	]
}
```

The "profiles" section contains a list of application profiles. Libglvnd will
scan that list and use the first element in each JSON file that matches the
current process.

If a matching profile is found in different files, then libglvnd will merge
them together.

Each profile is a JSON object with the following values:
* "match": An array of strings to match against the current process. As a
  convenience, a single string can be used instead of an array.
* "override": If this value is true, then libglvnd will ignore any later
  matches intead of merging them.
* "vendors": Contains a list of vendors to load.

Each vendor contains the following values:
* "vendor\_name": The name of the vendor library to load.
* "vendor\_data": An optional, arbitrary JSON tree which is passed to the
  vendor library. This can be used for any vendor-specific configuration, such
  as specifying a particular device to use.
* "only\_in\_server\_list": If true, then libglvnd will only load the vendor
  if the vendor name is included in the server's GLX\_VENDOR\_NAMES\_EXT string
  for the default screen. This is purely an optimization to avoid loading
  vendor libraries unnecessarily.
* "disable": If true, then libglvnd will ignore the vendor, including any
  later vendors with the same name. This allows a config file to disable a
  vendor entry defined in a later config file.

Alternately, if the value "rule\_name" is present, then libglvnd will use the
vendors defined in a separate rule file.

## Rule Files

Rules are sort of like macros which expand out to a list of vendors.

Each rule contains a list of vendors, just like you'd put in the app profile
itself. When a profile references a rule, then libglvnd will expand that
reference to the rule's full vendor list, as if they were all listed directly
in the profile.

Rules provide a way to define a generic class of application. For example, an
app profile could specify "performance", and then a driver could provide a rule
file to expand that to its own vendor name.

If the same rule name is defined more than once, then libglvnd will merge the
rule's vendor list the same way it would with an app profile.

Rule files are in the same directories as the profiles, but use the suffix
".rules.json".

The structure is largely the same as the profiles:
```JSON
{
	"version" : [ 0, 0 ],
	"rules" : [
		{
			"rule_name" : "performance",
			"vendor_name" : "nvidia"
		},
		{
			"rule_name" : "performance",
			"vendor_name" : "mesa",
			"vendor_data" : "pci-0000_00_02_0"
		}
	]
}
```

Each rule is a JSON object with the following values:
* "rule\_name": The name of the rule. This is used in an app profile to
  reference the rule.
* "override": If true, then libglvnd will ignore all later definitions of the
  rule. This allows a user to override a system-wide config file.
* "vendor\_name", "vendor\_data", "disable": These are the same as the values
  in an app profile.

## Merging Profiles

If two or more config files have a profile that matches the process, then
libglvnd will merge their vendor lists.

Libglvnd will use the vendors from each file, in the order that they are found.
But, if the same vendor name is used more than once, then libglvnd will only
keep the first one.

Note that libglvnd will only use the first matching profile from each config
file, so libglvnd will only merge matching profiles from different files.

## Profile Matching

Libglvnd currently matches a profile based on the executable name of the
current process.

The "match" item in each profile is a list of suffixes, which are matched
against the executable name. Only full path components are matched.

As a special case, if the string contains a leading "/", then libglvnd will
only match the entire path, not just a suffix.

For example, if the current process is `/usr/bin/glxgears`, then these strings
would match:
* "glxgears"
* "bin/glxgears"
* "/usr/bin/glxgears"

These strings would not match:
* "gears" -- Libglvnd won't match only part of a file or directory name
* "/bin/glxgears" -- With a leading '/', the entire path must match.

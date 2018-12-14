## Before this, we need to open Application Loader and log in to the
## right iTunes Connect account

echo "Not running - read, review, and update script first"
exit 1

xcrun altool --notarize-app -f "EasyMercurial-1.4.dmg" --primary-bundle-id org.easyhg.EasyMercurial -u "cannam+apple@all-day-breakfast.com" -p @keychain:"Application Loader: cannam+apple@all-day-breakfast.com"

## That churns for a while and then dumps out a UUID

# xcrun altool --notarization-info UUID -u "cannam+apple@all-day-breakfast.com" -p @keychain:"Application Loader: cannam+apple@all-day-breakfast.com"

## Returns "in progress" at first, then eventually a report with a URL
## that can be retrieved as JSON payload using wget. An email is also
## sent to the iTunes Connect account holder when it completes

# xcrun stapler staple -v "EasyMercurial-1.4.dmg"

# spctl -a -v "/Applications/EasyMercurial.app"


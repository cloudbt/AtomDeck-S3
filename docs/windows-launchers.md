# Windows launch cards

AtomDeck-S3 does not run shell commands and does not store application, file,
folder or URL paths. A launch card sends an allow-listed keyboard chord to a
Windows shortcut that the user has configured on an authorized PC.

## Configure Windows

1. Create a Windows shortcut for the application, file, folder or website.
2. Keep the shortcut on the Desktop or in the Start menu.
3. Open the shortcut's **Properties** window.
4. Select the **Shortcut key** field and press an unused combination such as
   `Ctrl+Alt+N`, then apply the change.
5. Test that combination directly on the target PC before configuring AtomDeck.

Avoid combinations used by Windows or other applications. Do not target scripts,
administrative tools, credential dialogs or untrusted downloads.

## Configure AtomDeck

1. Open **Customize** and select **Launch application / file / website**.
2. Choose a name, emoji and card color.
3. Set its chord to the same value, written as `CTRL+ALT+N`.
4. Unlock AtomDeck once with the physical button and save the card.
5. From the dashboard, tap the card to launch the Windows shortcut.

`GUI` represents the Windows key. AtomDeck supports up to three modifiers from
`CTRL`, `ALT`, `SHIFT` and `GUI`, followed by exactly one regular key.

function getMailFolder() {
	var REGKEY = "HKCU\\Software\\sn\\q3\\Setting\\MailFolder";
	var shell = new ActiveXObject("WScript.Shell");
	return shell.RegRead(REGKEY);
}

function loadAccounts(mailfolder) {
	var fso = new ActiveXObject("Scripting.FileSystemObject");
	var accounts = new ActiveXObject("MSXML2.DOMDocument.4.0");
	accounts.appendChild(accounts.createElement("accounts"));
	var accountfolder = fso.GetFolder(mailfolder + "\\accounts");
	var subfolders = new Enumerator(accountfolder.SubFolders);
	var n = 0;
	for (n = 0; !subfolders.atEnd(); subfolders.moveNext(), ++n) {
		var element = accounts.createElement("account");
		element.appendChild(accounts.createTextNode(subfolders.item().Name));
		accounts.firstChild.appendChild(element);
	}
	return accounts;
}

function loadXml(path) {
	var xml = new ActiveXObject("MSXML2.FreeThreadedDOMDocument.4.0");
	xml.async = false;
	xml.load(path);
	return xml;
}

function createXSLTemplate(xsl) {
	var template = new ActiveXObject("MSXML2.XSLTemplate.4.0");
	template.stylesheet = editXsl;
	return template;
}

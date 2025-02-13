A simple library for parsing xml in C++

Usage:
    Call the function readXML("example.xml").
    It will return the root node of the xml file.
    You can now extract the text and search for special tags by using root->findAllNodes("\<tagName>") or findNode for a single Node
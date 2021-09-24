/*
  Copyright © Matthew Perlman 2021

  Matthew Perlman
  Created 9/23/21

  XML Parser in C++

  This program generates nested xml objects from a raw xml string
*/

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

class ElementNode;

union AnyNode {
  std::string* text;
  ElementNode* element;
};

class Node;
class FlatNode;
std::vector<Node> toNodes(std::vector<FlatNode>);

class ElementNode
{
  public:
    std::string name;
    std::unordered_map<std::string, std::string> attributes;
    std::vector<Node> children;
    ElementNode()
    {
      name = "$";
    };
    ElementNode(std::string Name, std::unordered_map<std::string, std::string> Attributes, std::vector<FlatNode> Children)
    {
      name = Name;
      attributes = Attributes;
      children = toNodes(Children);
    };
};

class Node
{
  public:
    std::string type;
    AnyNode value;
    Node(std::string* Value)
    {
      type = "text";
      value.text = Value;
    };
    Node(ElementNode Value)
    {
      type = "element";
      value.element = new ElementNode(Value);
    };
    void print()
    {
      std::cout << std::endl;
      print(0);
      std::cout << std::endl;
    };
    void print(int indentation)
    {
      for(int i = 0;i<indentation;i++)
      {
        std::cout << " ";
      };
      if(type == "text"){
        std::cout << *value.text << std::endl;
      } else {
        std::cout << value.element->name + " (";
        std::string attributes;
        for(auto &attribute : value.element->attributes)
        {
          attributes += attribute.first + ": " + attribute.second + ", ";
        };
        if(value.element->attributes.size() > 0){
          attributes.erase(attributes.size() - 2, 2);
        };
        std::cout << attributes;
        std::cout << ")" << std::endl;
        for(int i = 0;i<value.element->children.size();i++)
        {
          value.element->children[i].print(indentation + 4);
        };
      };
    };
};

class FlatElementNode
{
  public:
    std::string name;
    std::unordered_map<std::string, std::string> attributes;
    bool isClosing;
    FlatElementNode(std::string tag)
    {
      std::string withoutTagIndicators = tag.substr(1, tag.size() - 2);
      while(withoutTagIndicators.size() > 0 && withoutTagIndicators.substr(0, 1) == " ")
      {
        withoutTagIndicators.erase(0, 1);
      };
      if(withoutTagIndicators.size() > 0 && withoutTagIndicators.substr(0, 1) == "/"){
        isClosing = true;
        withoutTagIndicators.erase(0, 1);
      } else {
        isClosing = false;
      };
      while(withoutTagIndicators.size() > 0 && withoutTagIndicators.substr(0, 1) != " ")
      {
        name += withoutTagIndicators.substr(0, 1);
        withoutTagIndicators.erase(0, 1);
      };
      std::string current;
      bool inString = false;
      bool inBrackets = false;
      while(withoutTagIndicators.size() > 0)
      {
        std::string c = withoutTagIndicators.substr(0, 1);
        current += c;
        if(c == "\""){
          inString = !inString;
        } else if(!inString){
          if(c == "{" && !inBrackets){
            inBrackets = true;
          } else if(c == "}" && inBrackets){
            inBrackets = false;
            while(current.size() > 0 && current.substr(0, 1) == " ")
            {
              current.erase(0, 1);
            };
            std::string attributeName;
            while(current.size() > 0 && current.substr(0, 1) != "=")
            {
              attributeName += current.substr(0, 1);
              current.erase(0, 1);
            };
            while(current.size() > 0 && current.substr(0, 1) != "{")
            {
              current.erase(0, 1);
            };
            std::string attributeValue = current.substr(1, current.size() - 2);
            attributes.insert(std::make_pair(attributeName, attributeValue));
            current = "";
          };
        };
        withoutTagIndicators.erase(0, 1);
      };
    }
};

union AnyFlatNode {
  std::string* text;
  FlatElementNode* element;
};

class FlatNode
{
  public:
    std::string type;
    AnyFlatNode value;
    FlatNode(std::string Value)
    {
      type = "text";
      value.text = new std::string(Value);
    };
    FlatNode(FlatElementNode Value)
    {
      type = "element";
      value.element = new FlatElementNode(Value);
    };
};

std::vector<FlatNode> toFlatNodes(std::string xml)
{
  std::vector<FlatNode> flatNodes;
  bool inString = false;
  std::string current;
  while(xml.size() > 0)
  {
    std::string c = xml.substr(0, 1);
    xml.erase(0, 1);
    if(current.size() != 0 || c != " "){
      if(c == "<"){
        if(current.size() != 0){
          flatNodes.push_back(FlatNode(current));
          current = c;
        } else {
          current += c;
        };
      } else {
        current += c;
        if(c == "\""){
          inString = !inString;
        } else if(!inString && c == ">"){
          flatNodes.push_back(FlatNode(FlatElementNode(current)));
          current = "";
        };
      };
    };
  };
  if(current.size() > 0){
    flatNodes.push_back(current);
  };
  return flatNodes;
};

std::vector<Node> toNodes(std::vector<FlatNode> flatNodes)
{
  std::vector<Node> nodes;
  while(flatNodes.size() > 0)
  {
    if(flatNodes[0].type == "text"){
      nodes.push_back(Node(flatNodes[0].value.text));
      flatNodes.erase(flatNodes.begin(), flatNodes.begin() + 1);
    } else {
      std::string tagName = flatNodes[0].value.element->name;
      std::vector<FlatNode> children;
      int i = 1;
      bool foundEndingTag = false;
      int tagNameDepth = 1;
      while(flatNodes.size() > i && !foundEndingTag)
      {
        if(flatNodes[i].type == "text" || flatNodes[i].value.element->name != tagName){
          children.push_back(flatNodes[i]);
        } else if(flatNodes[i].value.element->isClosing){
          tagNameDepth--;
          if(tagNameDepth == 0){
            foundEndingTag = true;
          } else {
            children.push_back(flatNodes[i]);
          };
        } else {
          tagNameDepth++;
          children.push_back(flatNodes[i]);
        };
        i++;
      };
      if(foundEndingTag){
        nodes.push_back(Node(ElementNode(tagName, flatNodes[0].value.element->attributes, children)));
      };
      flatNodes.erase(flatNodes.begin(), flatNodes.begin() + i);
    };
  };
  return nodes;
};

Node parse(std::string xml)
{
  Node root = Node(ElementNode());
  std::vector<FlatNode> flatNodes = toFlatNodes(xml);
  std::vector<Node> nodes = toNodes(flatNodes);
  root.value.element->children = nodes;
  return root;
};

int main() {
  Node parsed = parse(
    "<html><head><title>My Project</title></head><body><h1 class={\"myClass\"} onClick={doStuff}>Hello World</h1><div id={\"myDiv\"}><div class={\"emptyDiv\"}></div><p key={0}>as easy as</p><p key={1}>123abc</p></div></body></html>"
  );
  parsed.print();
  return 0;
};
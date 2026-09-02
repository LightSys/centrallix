class Test:
  def __init__(self, number, description):
    self.number = number
    self.description = description
    self.checks_order = []
    self.checks_map = {}
    
  def record_check(self, description, passed):
    if description not in self.checks_map:
      self.checks_order.append(description)
    self.checks_map[description] = passed
    
  def get_test_result(self):
    print(f"START TEST {self.number}")
    checks_len = len(self.checks_order)
    passed_checks = 0
    for desc in self.checks_order:
      passed = self.checks_map[desc]
      if passed:
        passed_checks += 1
      print(f"\tTest {desc} ... {'PASS' if passed else 'FAIL'}")
      
    passed_test = checks_len == passed_checks
    print(f"TEST {self.number} = {self.description} ({passed_checks}/{checks_len}) {'PASS' if passed_test else 'FAIL'}")
    return passed_test
    
  
class TestReporter:
  def __init__(self, component_name):
    self.name = component_name
    self.tests = {}
  
  def add_test(self, test_number, test_name):
    self.tests[test_number] = Test(test_number, test_name)
    # self.tests.append(Test(test_number, test_name))
  
  def record_check(self, test_number, description, passed):
    self.tests[test_number].record_check(description, passed)
    
  def print_report(self):
    all_passed = True
    for test in self.tests:
      test_result = self.tests[test].get_test_result()
      if not test_result:
        all_passed = False
    print(f"\n{self.name} Test {'PASS' if all_passed else 'FAIL'}")
    return all_passed
    
'''
Usage:

from test_reporter import TestReporter

reporter = TestReporter("Button")

reporter.add_test(1, "Hover behavior test")
reporter.record_check(1, "pointimage change", True)
reporter.record_check(1, "tristate border change", False)

reporter.add_test(2, "Click behavior test")
reporter.record_check(2, "click event updates label", True)

reporter.print_report()
'''
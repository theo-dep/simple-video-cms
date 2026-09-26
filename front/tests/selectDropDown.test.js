import { fireEvent, render, screen, waitFor } from '@testing-library/preact';
import { h } from 'preact';
import { describe, expect, it, vi, beforeEach } from 'vitest';

import { MultiSelectDropDown, SingleSelectEditableDropDown, MultiSelectEditableDropDown } from '../component/SelectDropDown.js';

// Helper to create option elements
function createOptions(options) {
  return options.map((opt) => h('option', { value: opt.value, selected: opt.selected }, opt.label || opt.value));
}

// Helper to open dropdown by clicking header
function openDropdown() {
  const header = document.querySelector('.select-dropdown-header');
  fireEvent.click(header);
}

// Helper to get option elements (excludes Select All row)
function getOptionElements() {
  return document.querySelectorAll('.select-dropdown-option:not(.select-dropdown-option-all)');
}

// Helper to get option by value
function getOptionByValue(value) {
  const options = getOptionElements();
  for (const opt of options) {
    if (opt.getAttribute('data-value') === value) {
      return opt;
    }
  }
  return null;
}

// Common options for tests
const testOptions = [
  { value: '1', label: 'Option 1' },
  { value: '2', label: 'Option 2' },
  { value: '3', label: 'Option 3' },
];

// Wait for next event loop tick to allow state updates
function tick() {
  return new Promise((resolve) => setTimeout(resolve, 0));
}

describe('MultiSelectDropDown', () => {
  beforeEach(() => {
    vi.restoreAllMocks();
  });

  describe('Rendering and props', () => {
    it('renders with default placeholder', () => {
      render(h(MultiSelectDropDown, { name: 'test' }, createOptions(testOptions)));
      expect(screen.getByText('Select item(s)')).toBeInTheDocument();
    });

    it('renders with custom placeholder', () => {
      render(h(MultiSelectDropDown, { name: 'test', placeholder: 'Choose items' }, createOptions(testOptions)));
      expect(screen.getByText('Choose items')).toBeInTheDocument();
    });

    it('renders with correct name prop', () => {
      const { container } = render(h(MultiSelectDropDown, { name: 'test-name' }, createOptions(testOptions)));
      const nativeSelect = container.querySelector('select[name="test-name"]');
      expect(nativeSelect).toBeInTheDocument();
    });

    it('renders all provided options', () => {
      render(h(MultiSelectDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      expect(getOptionElements().length).toBe(testOptions.length);
    });

    it('renders with empty children', () => {
      render(h(MultiSelectDropDown, { name: 'test' }, []));
      openDropdown();
      expect(getOptionElements().length).toBe(0);
    });

    it('passes ref correctly', () => {
      const ref = { current: null };
      render(h(MultiSelectDropDown, { name: 'test', ref }, createOptions(testOptions)));
      expect(ref.current).toBeTruthy();
      expect(typeof ref.current.clear).toBe('function');
    });

    it('renders with pre-selected options', () => {
      const optionsWithSelected = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2' },
        { value: '3', label: 'Option 3', selected: true },
      ];
      render(h(MultiSelectDropDown, { name: 'test' }, createOptions(optionsWithSelected)));

      const tags = document.querySelectorAll('.select-dropdown-tag');
      expect(tags.length).toBe(2);
      expect(tags[0]).toHaveTextContent('Option 1');
      expect(tags[1]).toHaveTextContent('Option 3');
    });

    it('calls onChange with pre-selected values on render', () => {
      const onChange = vi.fn();
      const optionsWithSelected = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2' },
        { value: '3', label: 'Option 3', selected: true },
      ];
      render(h(MultiSelectDropDown, { name: 'test', onChange }, createOptions(optionsWithSelected)));

      expect(onChange).toHaveBeenCalledWith(['1', '3']);
    });

    it('renders with single pre-selected option', () => {
      const optionsWithSelected = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2' },
        { value: '3', label: 'Option 3' },
      ];
      render(h(MultiSelectDropDown, { name: 'test' }, createOptions(optionsWithSelected)));

      const tags = document.querySelectorAll('.select-dropdown-tag');
      expect(tags.length).toBe(1);
      expect(tags[0]).toHaveTextContent('Option 1');
    });

    it('renders with no pre-selected options', () => {
      render(h(MultiSelectDropDown, { name: 'test' }, createOptions(testOptions)));

      const tags = document.querySelectorAll('.select-dropdown-tag');
      expect(tags.length).toBe(0);
    });

    it('handles mixed selected props with boolean false', () => {
      const optionsWithMixed = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2', selected: false },
        { value: '3', label: 'Option 3', selected: true },
      ];
      render(h(MultiSelectDropDown, { name: 'test' }, createOptions(optionsWithMixed)));

      const tags = document.querySelectorAll('.select-dropdown-tag');
      expect(tags.length).toBe(2);
    });
  });

  describe('Selection', () => {
    it('selects single option on click', () => {
      const onChange = vi.fn();
      render(h(MultiSelectDropDown, { name: 'test', onChange }, createOptions(testOptions)));

      openDropdown();
      fireEvent.click(getOptionByValue('1'));

      expect(onChange).toHaveBeenCalledWith(['1']);
    });

    it('selects multiple options on click', () => {
      const onChange = vi.fn();
      render(h(MultiSelectDropDown, { name: 'test', onChange }, createOptions(testOptions)));

      openDropdown();
      fireEvent.click(getOptionByValue('1'));
      fireEvent.click(getOptionByValue('2'));

      expect(onChange).toHaveBeenLastCalledWith(['1', '2']);
    });

    it('deselects option on second click', () => {
      const onChange = vi.fn();
      render(h(MultiSelectDropDown, { name: 'test', onChange }, createOptions(testOptions)));

      openDropdown();
      fireEvent.click(getOptionByValue('1'));
      fireEvent.click(getOptionByValue('1'));

      expect(onChange).toHaveBeenLastCalledWith([]);
    });

    it('selects all options with Select All', () => {
      const onChange = vi.fn();
      render(h(MultiSelectDropDown, { name: 'test', onChange }, createOptions(testOptions)));

      openDropdown();
      fireEvent.click(screen.getByText('Select All'));

      expect(onChange).toHaveBeenCalledWith(['1', '2', '3']);
    });

    it('deselects all when all selected and Select All clicked', () => {
      const onChange = vi.fn();
      render(h(MultiSelectDropDown, { name: 'test', onChange }, createOptions(testOptions)));

      openDropdown();
      fireEvent.click(screen.getByText('Select All'));
      fireEvent.click(screen.getByText('Select All'));

      expect(onChange).toHaveBeenLastCalledWith([]);
    });

    it('displays selected tags', () => {
      const onChange = vi.fn();
      render(h(MultiSelectDropDown, { name: 'test', onChange }, createOptions(testOptions)));

      openDropdown();
      fireEvent.click(getOptionByValue('1'));
      fireEvent.click(getOptionByValue('2'));

      const tags = document.querySelectorAll('.select-dropdown-tag');
      expect(tags.length).toBe(2);
    });

    it('removes selected tag on remove button click', () => {
      const onChange = vi.fn();
      render(h(MultiSelectDropDown, { name: 'test', onChange }, createOptions(testOptions)));

      openDropdown();
      fireEvent.click(getOptionByValue('1'));
      fireEvent.click(getOptionByValue('2'));

      const removeButtons = screen.getAllByLabelText(/^Remove/);
      fireEvent.click(removeButtons[0]);

      expect(onChange).toHaveBeenLastCalledWith(['2']);
    });

    it('has correct aria-expanded attribute', () => {
      const { container } = render(h(MultiSelectDropDown, { name: 'test' }, createOptions(testOptions)));
      const dropdown = container.querySelector('.select-dropdown');

      expect(dropdown.getAttribute('aria-expanded')).toBe('false');
      openDropdown();
      expect(dropdown.getAttribute('aria-expanded')).toBe('true');
    });
  });

  describe('Search', () => {
    it('has search input with correct placeholder', () => {
      render(h(MultiSelectDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      expect(screen.getByPlaceholderText('Search...')).toBeInTheDocument();
    });

    it('shows all options when search input is empty', () => {
      render(h(MultiSelectDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      const searchInput = screen.getByPlaceholderText('Search...');

      // Clear any existing text
      fireEvent.input(searchInput, { target: { value: '' } });

      expect(getOptionElements().length).toBe(testOptions.length);
    });

    it('hides Select All when not multiple', () => {
      // MultiSelectDropDown has multiple=true, so Select All should be visible
      render(h(MultiSelectDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      expect(screen.getByText('Select All')).toBeInTheDocument();
    });
  });

  describe('Edge cases', () => {
    it('handles undefined onChange', () => {
      render(h(MultiSelectDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      expect(() => fireEvent.click(getOptionByValue('1'))).not.toThrow();
    });

    it('handles empty name', () => {
      const { container } = render(h(MultiSelectDropDown, { name: '' }, createOptions(testOptions)));
      const nativeSelect = container.querySelector('select[name=""]');
      expect(nativeSelect).toBeInTheDocument();
    });

    it('handles options with same value', () => {
      const duplicateOptions = [
        { value: '1', label: 'Option 1' },
        { value: '1', label: 'Duplicate 1' },
        { value: '2', label: 'Option 2' },
      ];
      render(h(MultiSelectDropDown, { name: 'test' }, createOptions(duplicateOptions)));
      openDropdown();
      // Both options with value '1' should be rendered
      expect(getOptionElements().length).toBe(3);
    });

    it('handles options with undefined value', () => {
      const optionsWithUndefined = [
        { value: undefined, label: 'Undefined' },
        { value: '1', label: 'Option 1' },
      ];
      render(h(MultiSelectDropDown, { name: 'test' }, createOptions(optionsWithUndefined)));
      openDropdown();
      expect(getOptionElements().length).toBe(2);
    });

    it('removes pre-selected option via tag remove button', async () => {
      const onChange = vi.fn();
      const optionsWithSelected = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2', selected: true },
        { value: '3', label: 'Option 3' },
      ];
      render(h(MultiSelectDropDown, { name: 'test', onChange }, createOptions(optionsWithSelected)));

      const removeButtons = screen.getAllByLabelText(/^Remove/);
      fireEvent.click(removeButtons[0]);

      expect(onChange).toHaveBeenLastCalledWith(['2']);
    });

    it('removes all pre-selected options then adds new one', async () => {
      const onChange = vi.fn();
      const optionsWithSelected = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2', selected: true },
        { value: '3', label: 'Option 3' },
      ];
      render(h(MultiSelectDropDown, { name: 'test', onChange }, createOptions(optionsWithSelected)));

      const removeButtons = screen.getAllByLabelText(/^Remove/);
      fireEvent.click(removeButtons[0]);

      const remainingRemoveButtons = screen.getAllByLabelText(/^Remove/);
      fireEvent.click(remainingRemoveButtons[0]);

      openDropdown();
      await tick();
      fireEvent.click(getOptionByValue('3'));

      expect(onChange).toHaveBeenLastCalledWith(['3']);
    });
  });
});

describe('SingleSelectEditableDropDown', () => {
  beforeEach(() => {
    vi.restoreAllMocks();
  });

  describe('Rendering and props', () => {
    it('renders with default placeholder', () => {
      render(h(SingleSelectEditableDropDown, { name: 'test' }, createOptions(testOptions)));
      expect(screen.getByText('Select item')).toBeInTheDocument();
    });

    it('renders with custom placeholder', () => {
      render(h(SingleSelectEditableDropDown, { name: 'test', placeholder: 'Choose one' }, createOptions(testOptions)));
      expect(screen.getByText('Choose one')).toBeInTheDocument();
    });

    it('passes name prop to internal select', () => {
      const { container } = render(h(SingleSelectEditableDropDown, { name: 'test-name' }, createOptions(testOptions)));
      const nativeSelect = container.querySelector('select[name="test-name"]');
      expect(nativeSelect).toBeInTheDocument();
    });

    it('renders all options', () => {
      render(h(SingleSelectEditableDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      expect(getOptionElements().length).toBe(testOptions.length);
    });

    it('has search or add placeholder', () => {
      render(h(SingleSelectEditableDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      expect(screen.getByPlaceholderText('Search or add...')).toBeInTheDocument();
    });

    it('shows add button', () => {
      render(h(SingleSelectEditableDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      expect(screen.getByLabelText('Add option')).toBeInTheDocument();
    });

    it('shows edit buttons for options', () => {
      render(h(SingleSelectEditableDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      const editButtons = document.querySelectorAll('.select-dropdown-option-edit');
      expect(editButtons.length).toBeGreaterThan(0);
    });

    it('shows delete buttons for options', () => {
      render(h(SingleSelectEditableDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      const deleteButtons = document.querySelectorAll('.select-dropdown-option-delete');
      expect(deleteButtons.length).toBeGreaterThan(0);
    });

    it('renders with pre-selected option', () => {
      const optionsWithSelected = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2' },
        { value: '3', label: 'Option 3' },
      ];
      render(h(SingleSelectEditableDropDown, { name: 'test' }, createOptions(optionsWithSelected)));

      const tags = document.querySelectorAll('.select-dropdown-tag');
      expect(tags.length).toBe(1);
      expect(tags[0]).toHaveTextContent('Option 1');
    });

    it('calls onChange with pre-selected value on render', () => {
      const onChange = vi.fn();
      const optionsWithSelected = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2' },
        { value: '3', label: 'Option 3' },
      ];
      render(h(SingleSelectEditableDropDown, { name: 'test', onChange }, createOptions(optionsWithSelected)));

      expect(onChange).toHaveBeenCalledWith(['1']);
    });

    it('renders with last selected option when multiple have selected=true', () => {
      const optionsWithMultipleSelected = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2', selected: true },
        { value: '3', label: 'Option 3' },
      ];
      render(h(SingleSelectEditableDropDown, { name: 'test' }, createOptions(optionsWithMultipleSelected)));

      const tags = document.querySelectorAll('.select-dropdown-tag');
      expect(tags.length).toBe(1);
      expect(tags[0]).toHaveTextContent('Option 2');
    });

    it('handles selected=false explicitly', () => {
      const optionsWithMixed = [
        { value: '1', label: 'Option 1', selected: false },
        { value: '2', label: 'Option 2', selected: true },
        { value: '3', label: 'Option 3', selected: false },
      ];
      render(h(SingleSelectEditableDropDown, { name: 'test' }, createOptions(optionsWithMixed)));

      const tags = document.querySelectorAll('.select-dropdown-tag');
      expect(tags.length).toBe(1);
      expect(tags[0]).toHaveTextContent('Option 2');
    });
  });

  describe('Selection', () => {
    it('selects option on click', async () => {
      const onChange = vi.fn();
      render(h(SingleSelectEditableDropDown, { name: 'test', onChange }, createOptions(testOptions)));

      openDropdown();
      await tick();

      const option1 = getOptionByValue('1');
      expect(option1).toBeTruthy();

      // Click on the option div directly
      fireEvent.click(option1);

      // Wait for the state update
      await waitFor(() => {
        expect(onChange).toHaveBeenCalled();
      });
      expect(onChange).toHaveBeenCalledWith(['1']);
    });

    it('auto-closes after selection', () => {
      const onChange = vi.fn();
      render(h(SingleSelectEditableDropDown, { name: 'test', onChange }, createOptions(testOptions)));

      openDropdown();
      expect(screen.getByPlaceholderText('Search or add...')).toBeInTheDocument();

      fireEvent.click(getOptionByValue('1'));

      // After selection, dropdown should be closed
      expect(screen.queryByPlaceholderText('Search or add...')).not.toBeInTheDocument();
    });

    it('replaces selection when clicking different option', async () => {
      const onChange = vi.fn();
      render(h(SingleSelectEditableDropDown, { name: 'test', onChange }, createOptions(testOptions)));

      openDropdown();
      await tick();
      fireEvent.click(getOptionByValue('1'));

      openDropdown();
      await tick();
      fireEvent.click(getOptionByValue('2'));

      expect(onChange).toHaveBeenLastCalledWith(['2']);
    });
  });

  describe('Editing', () => {
    it('calls onAddedOption when adding new option via Enter', () => {
      const onChange = vi.fn();
      const onAddedOption = vi.fn().mockResolvedValue('new-value');
      render(h(SingleSelectEditableDropDown, { name: 'test', onChange, onAddedOption }, createOptions(testOptions)));

      openDropdown();
      const searchInput = screen.getByPlaceholderText('Search or add...');
      fireEvent.input(searchInput, { target: { value: 'New Option' } });
      fireEvent.keyDown(searchInput, { key: 'Enter' });

      expect(onAddedOption).toHaveBeenCalledWith('New Option');
    });

    it('calls onAddedOption when adding via add button', () => {
      const onChange = vi.fn();
      const onAddedOption = vi.fn().mockResolvedValue('new-value');
      render(h(SingleSelectEditableDropDown, { name: 'test', onChange, onAddedOption }, createOptions(testOptions)));

      openDropdown();
      const searchInput = screen.getByPlaceholderText('Search or add...');
      fireEvent.input(searchInput, { target: { value: 'New Option' } });
      fireEvent.click(screen.getByLabelText('Add option'));

      expect(onAddedOption).toHaveBeenCalledWith('New Option');
    });

    it('auto-selects new option when onAddedOption returns value', async () => {
      const onChange = vi.fn();
      const onAddedOption = vi.fn().mockResolvedValue('new-value');
      const { rerender } = render(h(SingleSelectEditableDropDown, { name: 'test', onChange, onAddedOption }, createOptions(testOptions)));

      openDropdown();
      await tick();
      const searchInput = screen.getByPlaceholderText('Search or add...');
      fireEvent.input(searchInput, { target: { value: 'New Option' } });
      fireEvent.keyDown(searchInput, { key: 'Enter' });

      // Wait for async onAddedOption to complete
      await tick();

      // Re-render with the new option added to children
      rerender(
        h(SingleSelectEditableDropDown, { name: 'test', onChange, onAddedOption }, [
          ...createOptions(testOptions),
          h('option', { value: 'new-value' }, 'New Option'),
        ])
      );

      await waitFor(() => {
        expect(onChange).toHaveBeenCalledWith(['new-value']);
      });
    });

    it('does not add when onAddedOption returns null', async () => {
      const onChange = vi.fn();
      const onAddedOption = vi.fn().mockResolvedValue(null);
      render(h(SingleSelectEditableDropDown, { name: 'test', onChange, onAddedOption }, createOptions(testOptions)));

      openDropdown();
      await tick();
      const searchInput = screen.getByPlaceholderText('Search or add...');
      fireEvent.input(searchInput, { target: { value: 'New Option' } });
      fireEvent.keyDown(searchInput, { key: 'Enter' });

      await tick();

      expect(onAddedOption).toHaveBeenCalledWith('New Option');
      // Since onAddedOption returned null, no option should be auto-selected
      // But onChange might still be called with empty array due to dropdown closing
      // So we check that it wasn't called with ['new-value']
      expect(onChange).not.toHaveBeenCalledWith(['new-value']);
    });

    it('calls onDeletedOption when deleting option', () => {
      const onChange = vi.fn();
      const onDeletedOption = vi.fn();
      render(h(SingleSelectEditableDropDown, { name: 'test', onChange, onDeletedOption }, createOptions(testOptions)));

      openDropdown();
      const deleteButtons = document.querySelectorAll('.select-dropdown-option-delete');
      fireEvent.click(deleteButtons[0]);

      expect(onDeletedOption).toHaveBeenCalledWith('1');
    });
  });

  describe('Edge cases', () => {
    it('handles undefined callbacks', () => {
      render(h(SingleSelectEditableDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      expect(screen.getByPlaceholderText('Search or add...')).toBeInTheDocument();
    });

    it('does not add empty string', () => {
      const onChange = vi.fn();
      const onAddedOption = vi.fn();
      render(h(SingleSelectEditableDropDown, { name: 'test', onChange, onAddedOption }, createOptions(testOptions)));

      openDropdown();
      const searchInput = screen.getByPlaceholderText('Search or add...');
      fireEvent.input(searchInput, { target: { value: '   ' } });
      fireEvent.keyDown(searchInput, { key: 'Enter' });

      expect(onAddedOption).not.toHaveBeenCalled();
    });

    it('removes pre-selected option via tag remove and deselects', async () => {
      const onChange = vi.fn();
      const optionsWithSelected = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2' },
        { value: '3', label: 'Option 3' },
      ];
      render(h(SingleSelectEditableDropDown, { name: 'test', onChange }, createOptions(optionsWithSelected)));

      const removeButtons = screen.getAllByLabelText(/^Remove/);
      fireEvent.click(removeButtons[0]);

      expect(onChange).toHaveBeenCalledWith([]);
    });
  });

  describe('Modification with pre-selection', () => {
    it('adds new option and replaces pre-selected', async () => {
      const onChange = vi.fn();
      const onAddedOption = vi.fn().mockResolvedValue('new-value');
      let options = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2' },
        { value: '3', label: 'Option 3' },
      ];
      const { rerender } = render(h(SingleSelectEditableDropDown, { name: 'test', onChange, onAddedOption }, createOptions(options)));

      openDropdown();
      await tick();
      const searchInput = screen.getByPlaceholderText('Search or add...');
      fireEvent.input(searchInput, { target: { value: 'New Option' } });
      fireEvent.keyDown(searchInput, { key: 'Enter' });

      await tick();

      options.push({ value: 'new-value', label: 'New Option' });
      rerender(h(SingleSelectEditableDropDown, { name: 'test', onChange, onAddedOption }, createOptions(options)));

      await waitFor(() => {
        expect(onChange).toHaveBeenCalledWith(['new-value']);
      });
    });

    it('deletes pre-selected option and clears selection', async () => {
      const onChange = vi.fn();
      const onDeletedOption = vi.fn();
      let options = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2' },
        { value: '3', label: 'Option 3' },
      ];
      const { rerender } = render(h(SingleSelectEditableDropDown, { name: 'test', onChange, onDeletedOption }, createOptions(options)));

      openDropdown();
      await tick();
      const deleteButtons = document.querySelectorAll('.select-dropdown-option-delete');
      fireEvent.click(deleteButtons[0]);

      options = options.filter((o) => o.value !== '1');
      rerender(h(SingleSelectEditableDropDown, { name: 'test', onChange, onDeletedOption }, createOptions(options)));

      expect(onDeletedOption).toHaveBeenCalledWith('1');
      expect(onChange).toHaveBeenLastCalledWith([]);
    });

    it('renames pre-selected option and maintains selection', async () => {
      const onChange = vi.fn();
      const onEditOption = vi.fn();
      let options = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2' },
        { value: '3', label: 'Option 3' },
      ];
      const { rerender } = render(h(SingleSelectEditableDropDown, { name: 'test', onChange, onEditOption }, createOptions(options)));

      openDropdown();
      await tick();
      const editButtons = document.querySelectorAll('.select-dropdown-option-edit');
      fireEvent.click(editButtons[0]);

      const editInput = document.querySelector('.select-dropdown-option-input');
      fireEvent.input(editInput, { target: { value: 'Renamed Option' } });
      fireEvent.keyDown(editInput, { key: 'Enter' });

      await tick();

      expect(onEditOption).toHaveBeenCalledWith('1', 'Renamed Option');

      options[0].label = 'Renamed Option';
      rerender(h(SingleSelectEditableDropDown, { name: 'test', onChange, onEditOption }, createOptions(options)));

      expect(onChange).toHaveBeenCalledWith(['1']);
    });
  });
});

describe('MultiSelectEditableDropDown', () => {
  beforeEach(() => {
    vi.restoreAllMocks();
  });

  describe('Rendering and props', () => {
    it('renders with default placeholder', () => {
      render(h(MultiSelectEditableDropDown, { name: 'test' }, createOptions(testOptions)));
      expect(screen.getByText('Select item(s)')).toBeInTheDocument();
    });

    it('renders with custom placeholder', () => {
      render(h(MultiSelectEditableDropDown, { name: 'test', placeholder: 'Choose items' }, createOptions(testOptions)));
      expect(screen.getByText('Choose items')).toBeInTheDocument();
    });

    it('passes name prop to internal select', () => {
      const { container } = render(h(MultiSelectEditableDropDown, { name: 'test-name' }, createOptions(testOptions)));
      const nativeSelect = container.querySelector('select[name="test-name"]');
      expect(nativeSelect).toBeInTheDocument();
    });

    it('renders all options', () => {
      render(h(MultiSelectEditableDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      expect(getOptionElements().length).toBe(testOptions.length);
    });

    it('has search or add placeholder', () => {
      render(h(MultiSelectEditableDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      expect(screen.getByPlaceholderText('Search or add...')).toBeInTheDocument();
    });

    it('shows add button', () => {
      render(h(MultiSelectEditableDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      expect(screen.getByLabelText('Add option')).toBeInTheDocument();
    });

    it('shows Select All button', () => {
      render(h(MultiSelectEditableDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      expect(screen.getByText('Select All')).toBeInTheDocument();
    });

    it('has edit and delete buttons for options', () => {
      render(h(MultiSelectEditableDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      const editButtons = document.querySelectorAll('.select-dropdown-option-edit');
      const deleteButtons = document.querySelectorAll('.select-dropdown-option-delete');
      expect(editButtons.length).toBeGreaterThan(0);
      expect(deleteButtons.length).toBeGreaterThan(0);
    });

    it('renders with pre-selected options', () => {
      const optionsWithSelected = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2' },
        { value: '3', label: 'Option 3', selected: true },
      ];
      render(h(MultiSelectEditableDropDown, { name: 'test' }, createOptions(optionsWithSelected)));

      const tags = document.querySelectorAll('.select-dropdown-tag');
      expect(tags.length).toBe(2);
      expect(tags[0]).toHaveTextContent('Option 1');
      expect(tags[1]).toHaveTextContent('Option 3');
    });

    it('calls onChange with pre-selected values on render', () => {
      const onChange = vi.fn();
      const optionsWithSelected = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2' },
        { value: '3', label: 'Option 3', selected: true },
      ];
      render(h(MultiSelectEditableDropDown, { name: 'test', onChange }, createOptions(optionsWithSelected)));

      expect(onChange).toHaveBeenCalledWith(['1', '3']);
    });

    it('renders with all options pre-selected', () => {
      const optionsAllSelected = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2', selected: true },
        { value: '3', label: 'Option 3', selected: true },
      ];
      render(h(MultiSelectEditableDropDown, { name: 'test' }, createOptions(optionsAllSelected)));

      const tags = document.querySelectorAll('.select-dropdown-tag');
      expect(tags.length).toBe(3);
    });

    it('handles pre-selected with string values', () => {
      const optionsWithStringValues = [
        { value: 'apple', label: 'Apple', selected: true },
        { value: 'banana', label: 'Banana' },
        { value: 'cherry', label: 'Cherry', selected: true },
      ];
      render(h(MultiSelectEditableDropDown, { name: 'test' }, createOptions(optionsWithStringValues)));

      const tags = document.querySelectorAll('.select-dropdown-tag');
      expect(tags.length).toBe(2);
      expect(tags[0]).toHaveTextContent('Apple');
      expect(tags[1]).toHaveTextContent('Cherry');
    });

    it('handles pre-selected with numeric string values', () => {
      const optionsWithNumericStrings = [
        { value: '0', label: 'Zero', selected: true },
        { value: '1', label: 'One' },
        { value: '2', label: 'Two', selected: true },
      ];
      render(h(MultiSelectEditableDropDown, { name: 'test' }, createOptions(optionsWithNumericStrings)));

      const tags = document.querySelectorAll('.select-dropdown-tag');
      expect(tags.length).toBe(2);
    });
  });

  describe('Selection', () => {
    it('selects multiple options on click', async () => {
      const onChange = vi.fn();
      render(h(MultiSelectEditableDropDown, { name: 'test', onChange }, createOptions(testOptions)));

      openDropdown();
      await tick();
      fireEvent.click(getOptionByValue('1'));
      fireEvent.click(getOptionByValue('2'));

      expect(onChange).toHaveBeenLastCalledWith(['1', '2']);
    });

    it('does not auto-close after selection', () => {
      const onChange = vi.fn();
      render(h(MultiSelectEditableDropDown, { name: 'test', onChange }, createOptions(testOptions)));

      openDropdown();
      fireEvent.click(getOptionByValue('1'));

      // Dropdown should still be open (search input visible)
      expect(screen.getByPlaceholderText('Search or add...')).toBeInTheDocument();
    });

    it('selects all options with Select All', async () => {
      const onChange = vi.fn();
      render(h(MultiSelectEditableDropDown, { name: 'test', onChange }, createOptions(testOptions)));

      openDropdown();
      await tick();
      fireEvent.click(screen.getByText('Select All'));

      expect(onChange).toHaveBeenCalledWith(['1', '2', '3']);
    });

    it('deselects all when all selected and Select All clicked', async () => {
      const onChange = vi.fn();
      render(h(MultiSelectEditableDropDown, { name: 'test', onChange }, createOptions(testOptions)));

      openDropdown();
      await tick();
      fireEvent.click(screen.getByText('Select All'));
      fireEvent.click(screen.getByText('Select All'));

      expect(onChange).toHaveBeenLastCalledWith([]);
    });
  });

  describe('Editing', () => {
    it('calls onAddedOption when adding new option via Enter', () => {
      const onChange = vi.fn();
      const onAddedOption = vi.fn().mockResolvedValue('new-value');
      render(h(MultiSelectEditableDropDown, { name: 'test', onChange, onAddedOption }, createOptions(testOptions)));

      openDropdown();
      const searchInput = screen.getByPlaceholderText('Search or add...');
      fireEvent.input(searchInput, { target: { value: 'New Option' } });
      fireEvent.keyDown(searchInput, { key: 'Enter' });

      expect(onAddedOption).toHaveBeenCalledWith('New Option');
    });

    it('calls onAddedOption when adding via add button', () => {
      const onChange = vi.fn();
      const onAddedOption = vi.fn().mockResolvedValue('new-value');
      render(h(MultiSelectEditableDropDown, { name: 'test', onChange, onAddedOption }, createOptions(testOptions)));

      openDropdown();
      const searchInput = screen.getByPlaceholderText('Search or add...');
      fireEvent.input(searchInput, { target: { value: 'New Option' } });
      fireEvent.click(screen.getByLabelText('Add option'));

      expect(onAddedOption).toHaveBeenCalledWith('New Option');
    });

    it('stays open after adding option', async () => {
      const onChange = vi.fn();
      const onAddedOption = vi.fn().mockResolvedValue('new-value');
      render(h(MultiSelectEditableDropDown, { name: 'test', onChange, onAddedOption }, createOptions(testOptions)));

      openDropdown();
      const searchInput = screen.getByPlaceholderText('Search or add...');
      fireEvent.input(searchInput, { target: { value: 'New Option' } });
      fireEvent.keyDown(searchInput, { key: 'Enter' });

      // Wait for async operation
      await tick();

      // Dropdown should still be open
      expect(screen.getByPlaceholderText('Search or add...')).toBeInTheDocument();
    });

    it('calls onDeletedOption when deleting option', () => {
      const onChange = vi.fn();
      const onDeletedOption = vi.fn();
      render(h(MultiSelectEditableDropDown, { name: 'test', onChange, onDeletedOption }, createOptions(testOptions)));

      openDropdown();
      const deleteButtons = document.querySelectorAll('.select-dropdown-option-delete');
      fireEvent.click(deleteButtons[0]);

      expect(onDeletedOption).toHaveBeenCalledWith('1');
    });

    it('deselects option when deleted if selected', async () => {
      const onChange = vi.fn();
      const onDeletedOption = vi.fn();
      render(h(MultiSelectEditableDropDown, { name: 'test', onChange, onDeletedOption }, createOptions(testOptions)));

      openDropdown();
      await tick();
      fireEvent.click(getOptionByValue('1'));
      fireEvent.click(getOptionByValue('2'));

      const deleteButtons = document.querySelectorAll('.select-dropdown-option-delete');
      fireEvent.click(deleteButtons[0]);

      expect(onChange).toHaveBeenLastCalledWith(['2']);
    });
  });

  describe('Edge cases', () => {
    it('handles undefined callbacks', () => {
      render(h(MultiSelectEditableDropDown, { name: 'test' }, createOptions(testOptions)));
      openDropdown();
      expect(screen.getByPlaceholderText('Search or add...')).toBeInTheDocument();
    });

    it('handles selection and deletion simultaneously', async () => {
      const onChange = vi.fn();
      const onDeletedOption = vi.fn();
      render(h(MultiSelectEditableDropDown, { name: 'test', onChange, onDeletedOption }, createOptions(testOptions)));

      openDropdown();
      await tick();
      fireEvent.click(getOptionByValue('1'));
      fireEvent.click(getOptionByValue('2'));

      const deleteButtons = document.querySelectorAll('.select-dropdown-option-delete');
      fireEvent.click(deleteButtons[0]);

      expect(onChange).toHaveBeenLastCalledWith(['2']);
      expect(onDeletedOption).toHaveBeenCalledWith('1');
    });

    it('removes pre-selected option via tag remove button', async () => {
      const onChange = vi.fn();
      const optionsWithSelected = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2', selected: true },
        { value: '3', label: 'Option 3' },
      ];
      render(h(MultiSelectEditableDropDown, { name: 'test', onChange }, createOptions(optionsWithSelected)));

      const removeButtons = screen.getAllByLabelText(/^Remove/);
      fireEvent.click(removeButtons[0]);

      expect(onChange).toHaveBeenLastCalledWith(['2']);
    });
  });

  describe('Modification with pre-selection', () => {
    it('adds new option and keeps pre-selected options', async () => {
      const onChange = vi.fn();
      const onAddedOption = vi.fn().mockResolvedValue('4');
      let options = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2', selected: true },
        { value: '3', label: 'Option 3' },
      ];
      const { rerender } = render(h(MultiSelectEditableDropDown, { name: 'test', onChange, onAddedOption }, createOptions(options)));

      openDropdown();
      await tick();
      const searchInput = screen.getByPlaceholderText('Search or add...');
      fireEvent.input(searchInput, { target: { value: 'New Option' } });
      fireEvent.keyDown(searchInput, { key: 'Enter' });

      await tick();

      options.push({ value: '4', label: 'New Option' });
      rerender(h(MultiSelectEditableDropDown, { name: 'test', onChange, onAddedOption }, createOptions(options)));

      await waitFor(() => {
        expect(onChange).toHaveBeenLastCalledWith(['1', '2', '4']);
      });
    });

    it('deletes pre-selected option and keeps others selected', async () => {
      const onChange = vi.fn();
      const onDeletedOption = vi.fn();
      let options = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2', selected: true },
        { value: '3', label: 'Option 3', selected: true },
      ];
      const { rerender } = render(h(MultiSelectEditableDropDown, { name: 'test', onChange, onDeletedOption }, createOptions(options)));

      openDropdown();
      await tick();
      const deleteButtons = document.querySelectorAll('.select-dropdown-option-delete');
      fireEvent.click(deleteButtons[1]);

      options = options.filter((o) => o.value !== '2');
      rerender(h(MultiSelectEditableDropDown, { name: 'test', onChange, onDeletedOption }, createOptions(options)));

      expect(onDeletedOption).toHaveBeenCalledWith('2');
      expect(onChange).toHaveBeenLastCalledWith(['1', '3']);
    });

    it('renames pre-selected option and maintains selection', async () => {
      const onChange = vi.fn();
      const onEditOption = vi.fn();
      let options = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2', selected: true },
        { value: '3', label: 'Option 3' },
      ];
      const { rerender } = render(h(MultiSelectEditableDropDown, { name: 'test', onChange, onEditOption }, createOptions(options)));

      openDropdown();
      await tick();
      const editButtons = document.querySelectorAll('.select-dropdown-option-edit');
      fireEvent.click(editButtons[0]);

      const editInput = document.querySelector('.select-dropdown-option-input');
      fireEvent.input(editInput, { target: { value: 'Renamed Option' } });
      fireEvent.keyDown(editInput, { key: 'Enter' });

      await tick();

      expect(onEditOption).toHaveBeenCalledWith('1', 'Renamed Option');

      options[0].label = 'Renamed Option';
      rerender(h(MultiSelectEditableDropDown, { name: 'test', onChange, onEditOption }, createOptions(options)));

      expect(onChange).toHaveBeenLastCalledWith(['1', '2']);
    });

    it('adds and removes pre-selected options in sequence', async () => {
      const onChange = vi.fn();
      const onDeletedOption = vi.fn();
      const onAddedOption = vi.fn().mockResolvedValue('4');
      let options = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2', selected: true },
        { value: '3', label: 'Option 3' },
      ];
      const { rerender } = render(h(MultiSelectEditableDropDown, { name: 'test', onChange, onDeletedOption, onAddedOption }, createOptions(options)));

      // Remove one pre-selected
      const removeButtons = screen.getAllByLabelText(/^Remove/);
      fireEvent.click(removeButtons[0]);

      // Add a new option
      openDropdown();
      await tick();
      const searchInput = screen.getByPlaceholderText('Search or add...');
      fireEvent.input(searchInput, { target: { value: 'New' } });
      fireEvent.keyDown(searchInput, { key: 'Enter' });

      await tick();

      options = options.filter((o) => o.value !== '1');
      options.push({ value: '4', label: 'New Option' });
      rerender(h(MultiSelectEditableDropDown, { name: 'test', onChange, onDeletedOption, onAddedOption }, createOptions(options)));

      await waitFor(() => {
        expect(onChange).toHaveBeenLastCalledWith(['2', '4']);
      });
    });
  });

  describe('User selection preservation', () => {
    it('preserves user deselection when options change', async () => {
      const onChange = vi.fn();
      let options = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2', selected: true },
        { value: '3', label: 'Option 3' },
      ];

      const { rerender } = render(h(MultiSelectDropDown, { name: 'test', onChange }, createOptions(options)));

      expect(onChange).toHaveBeenCalledWith(['1', '2']);

      const removeButtons = screen.getAllByLabelText(/^Remove/);
      fireEvent.click(removeButtons[0]);

      expect(onChange).toHaveBeenLastCalledWith(['2']);

      options = [
        { value: '1', label: 'Option 1', selected: true },
        { value: '2', label: 'Option 2', selected: true },
        { value: '3', label: 'Option 3 Updated' },
      ];

      rerender(h(MultiSelectDropDown, { name: 'test', onChange }, createOptions(options)));

      await waitFor(() => {
        const tags = document.querySelectorAll('.select-dropdown-tag');
        expect(tags.length).not.toBe(0);
      });

      expect(onChange).toHaveBeenLastCalledWith(['2']);

      const tags = document.querySelectorAll('.select-dropdown-tag');
      expect(tags.length).toBe(1);
      expect(tags[0]).toHaveTextContent('Option 2');
    });
  });
});

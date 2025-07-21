import { mount } from '@vue/test-utils';
import FormCard from '@/components/FormCard.vue';
import { describe, it, expect, beforeEach } from 'jest';

describe('FormCard.vue', () => {
  let propsData;
  let wrapper;

  beforeEach(() => {
    propsData = {
      modelValue: {
        id: 'test-id',
        title: 'Test Title',
        value: 'Test Value',
        remark: 'Test Remark',
        media: 'test.jpg',
        showTitle: true,
        showValue: true,
        showRemark: true,
        showMedia: true,
        titleFontSize: 16,
        valueFontSize: 16,
        remarkFontSize: 14,
        titleColor: '#333',
        valueColor: '#333',
        remarkColor: '#666'
      },
      editable: true
    };

    wrapper = mount(FormCard, {
      propsData,
      attachTo: document.body
    });
  });

  it('renders correctly with provided props', () => {
    expect(wrapper.find('.form-card').exists()).toBe(true);
    expect(wrapper.find('input').element.value).toBe('Test Title');
    expect(wrapper.find('textarea').element.value).toBe('Test Remark');
  });

  it('emits save event when save button is clicked', async () => {
    await wrapper.find('button[type="primary"]').trigger('click');
    expect(wrapper.emitted('save')).toBeTruthy();
    expect(wrapper.emitted('save')[0][0]).toEqual(propsData.modelValue);
  });

  it('validates form data correctly', async () => {
    // 测试标题为空的情况
    await wrapper.setProps({
      modelValue: {
        ...propsData.modelValue,
        title: ''
      }
    });

    await wrapper.find('button[type="primary"]').trigger('click');
    expect(wrapper.emitted('save')).toBeUndefined();

    // 测试标题超过最大长度的情况
    await wrapper.setProps({
      modelValue: {
        ...propsData.modelValue,
        title: 'a'.repeat(51)
      }
    });

    await wrapper.find('button[type="primary"]').trigger('click');
    expect(wrapper.emitted('save')).toBeUndefined();

    // 测试值为空的情况
    await wrapper.setProps({
      modelValue: {
        ...propsData.modelValue,
        value: ''
      }
    });

    await wrapper.find('button[type="primary"]').trigger('click');
    expect(wrapper.emitted('save')).toBeUndefined();

    // 测试值超过最大长度的情况
    await wrapper.setProps({
      modelValue: {
        ...propsData.modelValue,
        value: 'a'.repeat(101)
      }
    });

    await wrapper.find('button[type="primary"]').trigger('click');
    expect(wrapper.emitted('save')).toBeUndefined();

    // 测试有效数据
    await wrapper.setProps({
      modelValue: {
        ...propsData.modelValue,
        title: 'Valid Title',
        value: 'Valid Value'
      }
    });

    await wrapper.find('button[type="primary"]').trigger('click');
    expect(wrapper.emitted('save')).toBeTruthy();
  });

  it('updates form data correctly', async () => {
    // 修改标题
    await wrapper.find('input').setValue('New Title');
    expect(wrapper.emitted('update:modelValue')[0][0].title).toBe('New Title');

    // 修改值
    await wrapper.findAll('input').at(1).setValue('New Value');
    expect(wrapper.emitted('update:modelValue')[1][0].value).toBe('New Value');

    // 修改备注
    await wrapper.find('textarea').setValue('New Remark');
    expect(wrapper.emitted('update:modelValue')[2][0].remark).toBe('New Remark');
  });

  it('disables inputs when editable is false', () => {
    wrapper.setProps({ editable: false });
    wrapper.vm.$nextTick(() => {
      wrapper.findAll('input').wrappers.forEach(input => {
        expect(input.element.disabled).toBe(true);
      });
      wrapper.findAll('textarea').wrappers.forEach(textarea => {
        expect(textarea.element.disabled).toBe(true);
      });
      wrapper.findAll('select').wrappers.forEach(select => {
        expect(select.element.disabled).toBe(true);
      });
    });
  });

  it('emits delete event when delete button is clicked', async () => {
    await wrapper.find('button[type="danger"]').trigger('click');
    expect(wrapper.emitted('delete')).toBeTruthy();
  });
});